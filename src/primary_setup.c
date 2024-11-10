#include <primary_setup.h>
#include <stm32f091xc.h>


/*
swap_player: switch buttons and turn value
called by turn timer
no arguments
*/
void swap_player(void){
    //assume only called to switch the turn
    if(turn){
        //mask 6, unmask other 6
        //i will wire this to PC0-5
        turn = 0;
        EXTI->IMR = 0b111111;
    }
    else{
        //unmask 6, mask other 6
        turn = 1;
        EXTI->IMR = 0b111111000000;
    }
}

/* FUNCTION HEADER:
PWM: LEDs will fade from bright green to nothing. Until it reaches 5 seconds, 
then the LED will flash red every second to indicate a coutdown to 0
*/
void setup_led_array(void) {
    //Using TIM1 for PWM output
    //TIM1_CH1 - TIM1_CH4 --> PA8 - PA11

    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN; //Enable RCC clock for GPIOA
    RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN; //Enable clock for TIM1

    GPIOA -> MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10 | GPIO_MODER_MODER11); //Clearing PA8-PA11
    GPIOA -> MODER |= (GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1); //Setting PA8-PA11 as alternate function mode
    GPIOA -> AFR[1] |= 0x00002222; //Select AF2 for PA8-11

    TIM1 -> BDTR |= TIM_BDTR_MOE; //Enable the MOE bit in the TIM1 BDTR register

    TIM1 -> PSC = 4800 - 1; //Divide the clock by 4800 -> 10 kHz
    TIM1 -> ARR = 10000 - 1; //Setting the ARR to create 1 Hz !MIGHT NEED TO CHANGE!

    TIM1 -> CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);
    TIM1 -> CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC4M);
    TIM1 -> CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); //PWM Mode 1 for channel 1
    TIM1 -> CCMR1 |= (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1); //PWM Mode 1 for channel 2
    TIM1 -> CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1); //PWM Mode 1 for channel 3
    TIM1 -> CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1); //PWM Mode 1 for channel 4

    TIM1->DIER |= TIM_DIER_UIE; // Enable update interrupt

    TIM1 -> CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E); //Enable the four channel outputs
    TIM1 -> CR1 |= TIM_CR1_CEN; //Enable counter
    TIM1 -> CCR3 = 0; //Turn off the blue LED

    NVIC -> ISER[0] |= 1 << TIM1_BRK_UP_TRG_COM_IRQn; //Enable the interrupt for Timer 1

    TIM1 -> CCR2 = 10000; //Initializing the green LED
}

/*
  TIM1 -> CCR1 : RED
  TIM1 -> CCR2: GREEN
  TIM1 -> CCR3: BLUE
*/

#define WARNING_TIME 5
int32_t time_remaining = 60;
int32_t toggle = 1;

void TIM1_BRK_UP_TRG_COM_IRQHandler() {
    TIM1->SR &= ~TIM_SR_UIF; // Acknowledge interrupt

    if (time_remaining >= 0) {
        time_remaining--;

        if (time_remaining > WARNING_TIME) { //When time > 5 seconds
            TIM1 -> CCR1 = 0; //Turn off red
            TIM1 -> CCR2 = TIM1 -> CCR2 - 166; // 10,000/60
        }
        else if (time_remaining <= WARNING_TIME) { //Last 5 seconds
            TIM1 -> CCR2 = 0; //Turn of green
            TIM1 -> CCR1 = toggle ? 10000 : 0;
            toggle = !toggle;
        } 
    }
    else { //Round is over
        TIM1 -> CCR1 = 0;
        TIM1 -> CCR2 = 0;
        toggle = 1;

        //FIND A WAY TO TURN OFF THE GREEN LIGHT ONCE THE GAME IS OVER?
        TIM1->CR1 &= ~TIM_CR1_CEN; // Stop TIM1
    }
}

void start_new_round_LED(void) {
    TIM1 -> CNT = 0; // Reset counter
    time_remaining = 60; // Reset for the next round if needed
    TIM1 -> CCR1 = 0; //Turn of red LED
    TIM1 -> CCR2 = 10000; //Turn on the green LED
    TIM1 -> CR1 |= TIM_CR1_CEN; // Start or restart TIM1 counter
}


/* FUNCTION HEADER:
setup gpioc for exti interrupts on pushbuttons
*/
void setup_buttons(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    //requires gpio moder and gpio PUPDR
    //default state is ok for moder
    //then use pulldown
    GPIOC->PUPDR |= 0xAAAAAA;
    

    // pins 0 to 11 exti enable
    SYSCFG->EXTICR[0] |= 0x2222;
    SYSCFG->EXTICR[1] |= 0x2222;
    SYSCFG->EXTICR[2] |= 0x2222;
    // use rising trigger and unmask
    EXTI->RTSR |= 0b111111111111;
    EXTI->IMR |= 0b111111111111;

    //enable interrupts
    NVIC->ISER[0] |= 1 << EXTI0_1_IRQn;
    NVIC->ISER[0] |= 1 << EXTI2_3_IRQn;
    NVIC->ISER[0] |= 1 << EXTI4_15_IRQn;
}
/*
FUNCTION HEADER:
READ IN A BUTTON, FIND OUT WHICH BUTTON
SET A BIT OR SOMETHING IDK
GO BACK TO MAIN
*/

void setup_turn_timer(void){
    RCC -> APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7 -> PSC = (48000 - 1); //configures timer to go off every 1 min 
    TIM7 -> ARR = (1250 - 1);
    TIM7 -> DIER = TIM_DIER_UIE; //enables interrupt update
    NVIC -> ISER[0] = 1<<TIM7_IRQn; //enables timer interrupt
    TIM7 -> CR1 |= TIM_CR1_CEN;
}

void TIM7_IRQHandler(void){
    TIM7 -> SR &= ~TIM_SR_UIF; //registers interrupt
    setup_buttons();
}

void disable_turn_timer(void){
    TIM7 -> CR1 &= ~TIM_CR1_CEN; //turns off timer
    //TIM7->DIER &= ~TIM_DIER_UIE; //turns off update for interrupt, dno if needed
    NVIC -> ICER[0] = 1<<TIM7_IRQn; //turns off timer interrupt for NVIC
}
/*
To be written
Must do the following:
Acknowledge interrupt, isolate pressed button, indicate action to main
Possible dma?
*/
void EXTI0_1IRQHANDLER(){

}
void EXTI2_3_IRQHandler(){

}
void EXTI4_15_IRQHandler(){

}
//LCD Functions, used for both micros
void init_spi1_slow(){
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |= (GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);


    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL3 | GPIO_AFRL_AFSEL4 | GPIO_AFRL_AFSEL5);
    GPIOB->AFR[0] |= (0x00 << GPIO_AFRL_AFSEL3_Pos) | (0x00 << GPIO_AFRL_AFSEL4_Pos) | (0x00 << GPIO_AFRL_AFSEL5_Pos);


    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    SPI1->CR1 |= (0b111 << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;

    SPI1->CR2 |= (0b111 << SPI_CR2_DS_Pos);
    SPI1->CR2 |= SPI_CR2_FRXTH;


    SPI1->CR1 |= SPI_CR1_SPE;
}
void enable_sdcard(){
    GPIOB->BSRR |= (1 <<18); 
}
void disable_sdcard(){
    GPIOB->BSRR |= 1 << 2;
}
void init_sdcard_io(){

    init_spi1_slow();
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);
    GPIOB->MODER |=  0b01 << GPIO_MODER_MODER2_Pos;
    disable_sdcard();

}
void sdcard_io_high_speed(){
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |= (0b001 << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_SPE;
}
void init_lcd_spi(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER11 | GPIO_MODER_MODER10);  // Clear current mode
    GPIOB->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER10_0);
    init_spi1_slow();
    sdcard_io_high_speed();
}