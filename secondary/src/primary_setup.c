#include "primary_setup.h"
#include <stm32f091xc.h>
#include "lcd.h"
#include <stdint.h>

/*
swap_player: switch buttons and turn value
called by turn timer
no arguments
*/
void handle_key(char);
void draw_cursor(u8, u8);
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

    //SET UP TIM2
    RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2 -> PSC = 480 - 1; // 1 Mhz
    TIM2 -> ARR = 100000 - 1; // 1 Hz
    TIM2 -> DIER = TIM_DIER_UIE; 
    NVIC -> ISER[0] |= 1 << TIM2_IRQn; 
    TIM2 -> CR1 |= TIM_CR1_CEN; //Enable Timer 2
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
float x = 2400;

void TIM2_IRQHandler() {
    TIM2 -> SR &= ~TIM_SR_UIF; //Acknowledging interrupt

    if (time_remaining >= 0) {
        if (time_remaining <= 5){
            TIM1 -> CCR3 = 2399; //Turn of green
            TIM1 -> CCR1 = toggle ? 0 : 2399;
            toggle = !toggle;
        } 
        else if (x > 1) {
            x /= 1.183;
            TIM1 -> CCR3 = 2400 - x;
        }
        time_remaining--;
    } else {
        TIM1 -> CCR1 = 2399;
        TIM1 -> CCR3 = 2399;
        toggle = 1;

        TIM1->CR1 &= ~TIM_CR1_CEN; // Stop TIM1
        TIM2 -> CR1 &= ~TIM_CR1_CEN; // Stop TIM2
    }
}


void setup_dma(void) {
    RCC -> AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_Channel4 -> CPAR = (uint32_t)(& (SPI2 -> DR));
    DMA1_Channel4 -> CMAR = (uint32_t) (&bs_users);
    DMA1_Channel4 -> CNDTR = sizeof(users); //size of data, can be adjusted as necessary
    DMA1_Channel4 -> CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN | DMA_CCR_CIRC;           

   // NVIC->ISER[0] |= (1 << DMA1_Channel4_5_IRQn);
}

// void DMA1_Channel4_5_IRQHandler(void) {
//     if (DMA1->ISR & DMA_ISR_TCIF5) {
//         DMA1->IFCR |= DMA_IFCR_CTCIF5;
//     }
//     if (DMA1->ISR & DMA_ISR_TCIF4) {
//         DMA1->IFCR |= DMA_IFCR_CTCIF4;
//     }
// }

/* FUNCTION HEADER:
SPI communication between microprocessors
PB12 - PB14 are connected from master to PB12, PB13, and PB15 on slave
Using SPI2 
SHOULDNT IT BE PB15 TO PB15? MOSI TO MOSI
*/

void init_slave(void) {
    RCC -> APB1ENR |= RCC_APB1ENR_SPI2EN; //Enable clock for SPI2 channel

    //Enable the ports used for SPI2 
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; //Enabling clock for port B
    GPIOB -> MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15); //Clearing the bits
    GPIOB -> MODER |= (GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1); //Configuring PB12 to be alternate output
    //Select AF0 for PB12, 13, and 15 
    GPIOB -> AFR[1] &= ~(GPIO_AFRH_AFSEL12 | GPIO_AFRH_AFSEL13 | GPIO_AFRH_AFSEL15); 

    SPI2 -> CR1 &= ~SPI_CR1_SPE; //Disable the SPI2 Channel first
    SPI2 -> CR1 &= ~SPI_CR1_MSTR; //Congigure as slave
    SPI2->CR2 |= SPI_CR2_RXDMAEN;          
    SPI2->CR1 |= SPI_CR1_SPE; //Enable SPI2
}


/* FUNCTION HEADER:
setup gpioc for exti interrupts on pushbuttons
*/


/*
FUNCTION HEADER:
READ IN A BUTTON, FIND OUT WHICH BUTTON
SET A BIT OR SOMETHING IDK
GO BACK TO MAIN
*/

/*
To be written
Must do the following:
Acknowledge interrupt, isolate pressed button, indicate action to main
Possible dma?
*/


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
    LCD_DrawFillRectangle(0,0,240,240,LIGHTBLUE);
    //3 pixel wide line
    //short axis
    for(int i = 0; i < 240; i+=240/FIELD_WIDTH){ //integer division
        LCD_DrawFillRectangle(i,0,i+3,320, DARKBLUE);
    }
    //long axis
    for(int i = 0; i <= 240; i+=320/FIELD_HEIGHT){
        LCD_DrawFillRectangle(0,i,240,i+3, DARKBLUE);
    }
    LCD_DrawFillRectangle(0,238,240,320,BLACK);
    LCD_DrawFillRectangle(238,0,320,320,BLACK);
}

void square_set(u8 x, u8 y, player* p, player* p2){
    u8 x_real = (x*234 / FIELD_WIDTH);
    u8 y_real = (y*312 / FIELD_HEIGHT);
    if (*game_status == 0){ // placing ships
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if(x == p->x && y == p->y) {
            draw_cursor(x, y);
        }
        if(p->field[y][x] == 1){
            //circle for ship
            LCD_Circle(x_real+14,y_real+14,6,1,MAGENTA);
        }
    }
    if (*game_status == 2) { // player 1's turn (display enemy field)
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if(x == p->x && y == p->y) {
            draw_cursor(x, y);
        } else if(p2->field[y][x] == 2) {
            //blue X for miss
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,DARKBLUE);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,DARKBLUE);
        } else if(p2->field[y][x] == 3) {
            //Red X for hit
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,RED);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,RED);
        }
    }
    if (*game_status == 1) {
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if(p->field[y][x] == 1) {
            //purple circle for ship
            LCD_Circle(x_real+14,y_real+14,6,1,MAGENTA);
        } else if(p->field[y][x] == 3) {
            //red circle for hit ship
            LCD_Circle(x_real+14,y_real+14,6,1,BLACK);
        } else if(p->field[y][x] == 4) {
            //blue X for missed
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,DARKBLUE);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,DARKBLUE);
        }
    }
}
void draw_cursor(u8 x, u8 y){
    //a + of 2 lines, should probably be drawn on top of the ship piece
    u8 x_real = (x*234 / FIELD_WIDTH);
    u8 y_real = (y*312 / FIELD_HEIGHT);
    LCD_DrawFillRectangle(x_real+13,y_real,x_real+15,y_real+26,DARKBLUE);
    LCD_DrawFillRectangle(x_real,y_real+13,x_real+26,y_real+15,DARKBLUE);
}

void update_grid() {
    for(int i = 0; i< FIELD_WIDTH;i++){
        for(int j=0; j<FIELD_WIDTH;j++){
            square_set(i,j,player_2, player_1);
        }
    }
    if(*game_status == 0) {
        draw_ship_tbp(player_2);
    }
    if(*game_status == 1) {
        LCD_DrawString(0, 240, WHITE, BLACK, "Player One's Turn", 16, 0);
    }
    else if(*game_status == 2) {
        LCD_DrawString(0, 240, WHITE, BLACK, "Player Two's Turn", 16, 0);
    }
    else if(*game_status >= 3) {
        display_stats();
    }
}

int draw_ship_tbp(player* p) {
    int x_real, y_real;
    if(p->rotation) { // placing horizontal ship
        for(int i = 0; i < ship_size[p->ship_index]; i++) {
            x_real = ((p->x+i) * 232) / FIELD_WIDTH;
            y_real = p->y * 312 / FIELD_HEIGHT;
            LCD_Circle(x_real+14,y_real+14,4,1,YELLOW);
        }
    }
    else { // placing vertical ship
        for(int i = 0; i < ship_size[p->ship_index]; i++) {
            x_real = ((p->x) * 232) / FIELD_WIDTH;
            y_real = (p->y + i) * 312 / FIELD_HEIGHT;
            LCD_Circle(x_real+14,y_real+14,4,1,YELLOW);
        }
    }
}

void display_stats(void) {
    LCD_DrawFillRectangle(0,238,240,256,BLACK);
    if(*game_status == 3) LCD_DrawString(0, 240, WHITE, BLACK, "Player One Wins", 16, 1);
    if(*game_status == 4) LCD_DrawString(0, 240, WHITE, BLACK, "Player Two Wins", 16, 1);
    if(*game_status == 5) LCD_DrawString(0, 240, WHITE, BLACK, "Tie Game", 16, 1);
    
    char bugger[50];
    sprintf(bugger, "Player Two Accuracy: %d%%", (100 * player_2->total_hits)/player_2->total_shots);
    LCD_DrawString(0, 256, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Player Two Max Hit Streak: %d", player_2->max_hit_streak);
    LCD_DrawString(0, 272, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Player Two Max Miss Streak: %d", player_2->max_miss_streak);
    LCD_DrawString(0, 288, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Number of rounds: %d", bs_users.num_rounds);
    LCD_DrawString(0, 304, WHITE, BLACK, bugger, 16, 1);
}

