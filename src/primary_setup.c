#include "primary_setup.h"
#include <stm32f091xc.h>
#include "lcd.h"
#include <stdint.h>
volatile uint8_t dmaData; //variable to store data
uint8_t txBuffer[128];

/*
swap_player: switch buttons and turn value
called by turn timer
no arguments
*/
void swap_player(void){
    //assume only called to switch the turn
    if(game_status == 1){
        //mask 6, unmask other 6
        //player 2 now gets to move
        EXTI->IMR = 0b111111;
    }
    else if (game_status == 2){
        //unmask 6, mask other 6
        //player 1 can now move
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

    TIM1 -> PSC = 1 - 1; //Divide the clock by 4800 -> 10 kHz
    TIM1 -> ARR = 2400 - 1; //Setting the ARR to create 1 Hz !MIGHT NEED TO CHANGE!

    TIM1 -> CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);
    TIM1 -> CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC4M);
    TIM1 -> CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); //PWM Mode 1 for channel 1
    TIM1 -> CCMR1 |= (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1); //PWM Mode 1 for channel 2
    TIM1 -> CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1); //PWM Mode 1 for channel 3
    TIM1 -> CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1);

    // TIM1->DIER |= TIM_DIER_UIE; // Enable update interrupt

    TIM1 -> CCR1 = 2399; //Turn off all LEDS
    TIM1 -> CCR2 = 2399;
    TIM1 -> CCR3 = 2399; 

    TIM1 -> CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E); //Enable the four channel outputs
    NVIC -> ISER[0] |= 1 << TIM1_BRK_UP_TRG_COM_IRQn; //Enable the interrupt for Timer 1

    TIM1 -> CR1 |= TIM_CR1_CEN; //Enable TIM1
}

/*
  TIM1 -> CCR1 : RED
  TIM1 -> CCR2: BLUE 
  TIM1 -> CCR3: GREEN

  0 -> ON
  2400 -> OFF
*/
#define WARNING_TIME 5
int32_t time_remaining = 60;
int32_t toggle = 1;

void startPWM() {
        
    for(float x=2400; x>1; x /= 1.183) {
        TIM1 -> CCR3 = 2400 - x;
        nano_wait(10000000);
    }

    TIM1 -> CCR3 = 2399;
    int ccr1Val;
    
    for (int y = 0; y < 5; y++) {
        if (toggle) {
            ccr1Val = 0;
        } else {
            ccr1Val = 2399;
        }
        TIM1 -> CCR1 = ccr1Val;
        toggle = !toggle;
        nano_wait(1000000000);
    }
    
    TIM1 -> CCR1 = 2399; //Turn off all LEDS
    TIM1 -> CCR2 = 2399;
    TIM1 -> CCR3 = 2399; 

    // TIM1 -> CR1 &= ~TIM_CR1_CEN; // Stop TIM1
}

void start_new_round_LED(void) {
    TIM1 -> CNT = 0; // Reset counter
    time_remaining = 60; // Reset for the next round if needed
    TIM1 -> CCR1 = 2399; //Turn of red LED
    TIM1 -> CCR3 = 0; //Turn on the green LED
    TIM1 -> CR1 |= TIM_CR1_CEN; // Start or restart TIM1 counter
}


void setup_dma(void) {
    RCC -> AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5 -> CPAR = (uint32_t)(& (SPI2 -> DR));
    DMA1_Channel5 -> CMAR = (uint32_t) txBuffer;
    DMA1_Channel5 -> CNDTR = sizeof(txBuffer);
    DMA1_Channel5 -> CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_EN;

    DMA1_Channel4 -> CPAR = (uint32_t)(& (SPI2 -> DR));
    DMA1_Channel4 -> CMAR = (uint32_t) (& dmaData);
    DMA1_Channel4 -> CNDTR = 128; //size of data, can be adjusted as necessary
    DMA1_Channel4 -> CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN;           
    SPI2->CR1 |= SPI_CR1_SPE; //Enable SPI2

    NVIC->ISER[0] |= (1 << DMA1_Channel4_5_IRQn);
}

void DMA1_Channel4_5_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_TCIF5) {
        DMA1->IFCR |= DMA_IFCR_CTCIF5;
    }
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR |= DMA_IFCR_CTCIF4;
    }
}

/* FUNCTION HEADER:
SPI communication between microprocessors
PB12 - PB14 are connected from master to PB12, PB13, and PB15 on slave
Using SPI2 
SHOULDNT IT BE PB15 TO PB15? MOSI TO MOSI
*/
void init_master(void) {
    RCC -> APB1ENR |= RCC_APB1ENR_SPI2EN; //Enable clock for SPI2 channel

    //Enable the ports used for SPI2 
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; //Enabling clock for port B
    GPIOB -> MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15); //Clearing the bits
    GPIOB -> MODER |= (GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1); //Configuring PB12 to be alternate output
    //Select AF0 for PB12, 13, and 15 
    GPIOB -> AFR[1] &= ~(GPIO_AFRH_AFSEL12 | GPIO_AFRH_AFSEL13 | GPIO_AFRH_AFSEL15); 

    SPI2 -> CR1 &= ~SPI_CR1_SPE; //Disable the SPI2 Channel first
    SPI2 -> CR1 |= (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2); //Set the baud rate as low as possible (Dividing by 256)

    SPI2 -> CR2 = (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_DS_3); //Setting data size to 16 bits (NEED TO CHANGE?)
    SPI2 -> CR1 |= SPI_CR1_MSTR; //Configure the SPI channel to be in "master configuration"
    SPI2 -> CR2 |= (SPI_CR2_SSOE | SPI_CR2_NSSP); //Enable SS Output enable bit and enable NSSP
    SPI2 -> CR2 |= SPI_CR2_TXDMAEN; //Set the TXDMAEN bit to enable DMA transfers on transmit buffer empty
    SPI2 -> CR1 |= SPI_CR1_SPE; //Enable the SPI2 Channel
}

void init_slave(void) {
    RCC -> APB1ENR |= RCC_APB1ENR_SPI2EN; //Enable clock for SPI2 channel

    //Enable the ports used for SPI2 
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; //Enabling clock for port B
    GPIOB -> MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15); //Clearing the bits
    GPIOB -> MODER |= (GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1); //Configuring PB12 to be alternate output
    //Select AF0 for PB12, 13, and 15 
    GPIOB -> AFR[1] &= ~(GPIO_AFRH_AFSEL12 | GPIO_AFRH_AFSEL13 | GPIO_AFRH_AFSEL15); 

    SPI2 -> CR1 &= ~SPI_CR1_SPE; //Disable the SPI2 Channel first
    SPI2 -> CR2 = (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_DS_3); //Setting data size to 16 bits (NEED TO CHANGE?)
    SPI2 -> CR1 &= ~SPI_CR1_MSTR; //Congigure as slave
    SPI2->CR2 |= SPI_CR2_RXDMAEN;          
    SPI2->CR1 |= SPI_CR1_SPE; //Enable SPI2
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

void setup_turn_timer(){
    RCC -> APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7 -> DIER |= TIM_DIER_UIE; //enables interrupt update
    TIM7 -> CR1 &= ~TIM_CR1_CEN;
    TIM7->CNT = 0;
    NVIC -> ISER[0] |= 1<<TIM7_IRQn; //enables timer interrupt
    TIM7 -> PSC = (48000 - 1); //configures timer to go off every 1 min


    timer_set = 0;
    TIM7 -> ARR = 1*1000 - 1;

    timer_set = 1;
    TIM7 -> CR1 |= TIM_CR1_CEN;
}

void start_timer(int tim) {
    TIM7 -> ARR = tim*1000 - 1;

    timer_set = 1;
    TIM7 -> CR1 |= TIM_CR1_CEN;

}

void TIM7_IRQHandler(void){
    TIM7 -> SR &= ~TIM_SR_UIF; //registers interrupt
    timer_set = 0;
    TIM7 -> CR1 &= ~TIM_CR1_CEN;
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
//pc0,pc1
//owned by player 2
if(EXTI->PR & 0b1){
    EXTI->PR = EXTI_PR_PR0;
    player_2.input = 0b1;
}
else if (EXTI->PR & (0b1 << 1))
{
    EXTI->PR = EXTI_PR_PR1;
    player_2.input = 0b1 <<1;
}
}
void EXTI2_3_IRQHandler(){
//pc2,3 owned by player 2
if(EXTI->PR & 0b1<<2){
    EXTI->PR = EXTI_PR_PR2;
    player_2.input = 0b1<<2;
}
else if (EXTI->PR & (0b1 << 3))
{
    EXTI->PR = EXTI_PR_PR3;
    player_2.input = 0b1 <<3;
}
}
void EXTI4_15_IRQHandler(){
//pc4,5, owned by player 2
if(EXTI->PR & 0b1<<4){
    EXTI->PR = EXTI_PR_PR4;
    player_2.input = 0b1<<4;
    return;
}
else if (EXTI->PR & (0b1 << 5))
{
    EXTI->PR = EXTI_PR_PR5;
    player_2.input = 0b1 <<5;
    return;
}
//pc 6,7,8,9,10,11 owned by player 1

if(EXTI->PR & 0b1<<6){
    EXTI->PR = EXTI_PR_PR6;
    player_1.input = 0b1;
}
else if (EXTI->PR & (0b1 << 7))
{
    EXTI->PR = EXTI_PR_PR7;
    player_1.input = 0b1 <<1;
}
else if (EXTI->PR & (0b1 << 8))
{
    EXTI->PR = EXTI_PR_PR8;
    player_1.input = 0b1 <<2;
}
else if (EXTI->PR & (0b1 << 9))
{
    EXTI->PR = EXTI_PR_PR9;
    player_1.input = 0b1 <<3;
}
else if (EXTI->PR & (0b1 << 10))
{
    EXTI->PR = EXTI_PR_PR10;
    player_1.input = 0b1 <<4;
}
else if (EXTI->PR & (0b1 << 11))
{
    EXTI->PR = EXTI_PR_PR11;
    player_1.input = 0b1 <<5;
}

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

/*
DRAWING UTILITIES
*/

void mapgen(void){
    //240 by 320 pixel map
    LCD_DrawFillRectangle(0,0,240,320,LIGHTBLUE);
    //3 pixel wide line
    //short axis
    for(int i = 0; i < 240; i+=240/FIELD_WIDTH){ //integer division
        LCD_DrawFillRectangle(i,0,i+3,320, DARKBLUE);
    }
    //long axis
    for(int i = 0; i <= 320; i+=320/FIELD_HEIGHT){
        LCD_DrawFillRectangle(0,i,240,i+3, DARKBLUE);
    }
}

void square_clear(u8 x, u8 y, player* p){
    u8 x_real = (x*240 / FIELD_WIDTH);
    u8 y_real = (y*320 / FIELD_HEIGHT);
    if(p->field[p->y][p->x] == 0){
    //clear space
    LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
    } else if(p->field[p->y][p->x] == 1){
        //circle for ship
    }else if(p->field[p->y][p->x] == 2){
        //blue X for miss
    }else if(p->field[p->y][p->x] == 3){
        //Orang Circle for dead ship bit
    }
}
void draw_cursor(u8 x, u8 y){
    //a + of 2 lines, should probably be drawn on top of the ship piece
    u8 x_real = (x*240 / FIELD_WIDTH);
    u8 y_real = (y*320 / FIELD_HEIGHT);
    LCD_DrawFillRectangle(x_real+13,y_real,x_real+15,y_real+26,DARKBLUE);
    LCD_DrawFillRectangle(x_real,y_real+13,x_real+26,y_real+15,DARKBLUE);
}