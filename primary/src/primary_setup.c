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
void display_stats(void);
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
    //NVIC -> ISER[0] |= 1 << TIM1_BRK_UP_TRG_COM_IRQn; //Enable the interrupt for Timer 1

    //TIM1 -> CR1 |= TIM_CR1_CEN; //Enable TIM1

    //SET UP TIM2
    RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2 -> PSC = 480 - 1; // 1 Mhz
    TIM2 -> ARR = 100000 - 1; // 1 Hz
    TIM2 -> DIER = TIM_DIER_UIE; 
    NVIC -> ISER[0] |= 1 << TIM2_IRQn; 
    //TIM2 -> CR1 |= TIM_CR1_CEN; //Enable Timer 2
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
int32_t toggle = 0;
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

void start_new_round_LED(void) {
    TIM1 -> CNT = 0; // Reset counter
    time_remaining = 60; // Reset for the next round if needed
    TIM1 -> CCR1 = 2400; //Turn of red LED
    TIM1 -> CCR3 = 0; //Turn on the green LED
    x = 2400; 
    toggle = 0;

    TIM1 -> CR1 |= TIM_CR1_CEN; //Enable TIM1
    TIM2 -> CR1 |= TIM_CR1_CEN; // Start or restart TIM2 counter
}

void stop_LED (void) {
    TIM1 -> CCR1 = 2400; //Turn of red LED
    TIM1 -> CCR2 = 2400;
    TIM1 -> CCR3 = 2400; //Turn on the green LED

    TIM1 -> CR1 &= ~TIM_CR1_CEN;
    TIM2 -> CR1 &= ~TIM_CR1_CEN; // Turn off TIM1 counter
}


void setup_dma(void) {
    RCC -> AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_Channel5 -> CPAR = (uint32_t)(& (SPI2 -> DR));
    DMA1_Channel5 -> CMAR = (uint32_t) &bs_users;
    DMA1_Channel5 -> CNDTR = sizeof(users);
    DMA1_Channel5 -> CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_EN | DMA_CCR_CIRC;

    // DMA1_Channel4 -> CPAR = (uint32_t)(& (SPI2 -> DR));
    // DMA1_Channel4 -> CMAR = (uint32_t) (& dmaData);
    // DMA1_Channel4 -> CNDTR = 128; //size of data, can be adjusted as necessary
    // DMA1_Channel4 -> CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN | DMA_CCR_CIRC;           

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
    TIM7->CNT = 0;

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
    } else if (*game_status == 1) { // player 1's turn (display enemy field)
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if((x == p->x && y == p->y)) {
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
    } else if (*game_status == 2) {
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if(p->field[y][x] == 1) {
            //purple circle for ship
            LCD_Circle(x_real+14,y_real+14,6,1,MAGENTA);
        } else if(p->field[y][x] == 3) {
            //red circle for hit ship
            LCD_Circle(x_real+14,y_real+14,6,1,BLACK);
        } else if(p->field[y][x] == 2) {
            //blue X for missed
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,DARKBLUE);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,DARKBLUE);
        }
    } else if (game_status >= 3) { // player 1's turn (display enemy field)
        LCD_DrawFillRectangle(x_real+4,y_real+4,x_real+25,y_real+25,LIGHTBLUE);
        if(p2->field[y][x] == 2) {
            //blue X for miss
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,DARKBLUE);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,DARKBLUE);
        } else if(p2->field[y][x] == 3) {
            //Red X for hit
            LCD_DrawLine(x_real,y_real,x_real+26,y_real+26,RED);
            LCD_DrawLine(x_real+26,y_real,x_real,y_real+26,RED);
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

//keypad utility from labs

uint8_t hist[16];
char queue[2];  // A two-entry queue of button press/release events.
int qin;        // Which queue entry is next for input
int qout;       // Which queue entry is next for output

const char keymap[] = "DCBA#9630852*741";

void push_queue(int n) {
    queue[qin] = n;
    qin ^= 1;
}

char pop_queue() {
    char tmp = queue[qout];
    queue[qout] = 0;
    qout ^= 1;
    return tmp;
}

void update_history(int c, int rows)
{
    // We used to make students do this in assembly language.
    for(int i = 0; i < 4; i++) {
        hist[4*c+i] = (hist[4*c+i]<<1) + ((rows>>i)&1);
        if (hist[4*c+i] == 0x01)
            push_queue(0x80 | keymap[4*c+i]);
        if (hist[4*c+i] == 0xfe)
            push_queue(keymap[4*c+i]);
    }
}

void drive_column(int c)
{
    c &= 0x3;
    GPIOC->BSRR |= 0xF << 20;
    GPIOC->BSRR |= 1 << (4+c);
}

int read_rows()
{
    return (GPIOC->IDR) & 0xf;
}

char get_key_event(void) {
    for(;;) {
        asm volatile ("wfi");   // wait for an interrupt
        if (queue[qout] != 0)
            break;
    }
    return pop_queue();
}

char get_keypress() {
    char event;
    for(;;) {
        // Wait for every button event...
        event = get_key_event();
        // ...but ignore if it's a release.
        if (event & 0x80)
            break;
    }
    return event & 0x7f;
}


char* keymap_arr = &keymap;
char rows_to_key(int rows) {
  rows &= 0xF;
  drive_column(col & 0b11);
      if((rows & 0b0001) == 1){
        return keymap_arr[(4*(col & 0b11))];
      }
      else if((rows & 0b0010) == 2){
        return keymap_arr[1 + 4*(col & 0b11)];
      }
      else if((rows & 0b0100) == 4){
        return keymap_arr[2 + 4*(col & 0b11)];
      }
      else if((rows & 0b1000) == 8){
        return keymap_arr[(3 + 4*(col & 0b11))];
      }
}
void TIM14_IRQHandler(){
  TIM14->SR &= ~TIM_SR_UIF;
  int rows = read_rows();
  if(rows != 0){
    char key = rows_to_key(rows);
    handle_key(key);
  }
  col++;
  if(col > 7){
    col = 0;
  }
  drive_column(col);
}
void handle_key(char key){
//global player access

//to reuse code maybe
//2->up
//5->down
//4->left
//6->right
//a fire
// b rot
    switch (key)
    {
        case '2':
        player_1->input = 1 << 5;
        break; case '4':
        player_1->input = 1 << 3;
        break;case '5':
        player_1->input = 1 << 4;
        break;case '6':
        player_1->input = 1 << 2;
        break;case 'B':
        player_1->input = 1 << 1;
        break;case 'A':
        player_1->input = 1 << 0;
        break;case '8':
        player_2->input = 1 << 5;
        break;case '*':
        player_2->input = 1 << 3;
        break;case '0':
        player_2->input = 1 << 4;
        break;case '#':
        player_2->input = 1 << 2;
        break;case 'D':
        player_2->input = 1 << 1;
        break;case 'C':
        player_2->input = 1 << 0;
        break;
    }

}
void setup_tim14() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    TIM14->PSC = 4800-1;//4799; 
    TIM14->ARR = 250-1;//9;
    TIM14->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= 1<< TIM14_IRQn;
    TIM14->CR1 |= TIM_CR1_CEN;
    
}
void enable_gpioC(){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    /*
    Configures pins PC4 – PC7 to be outputs
Configures pins PC0 – PC3 to be inputs
Configures pins PC0 – PC3 to be internally pulled low

Repeat PC8-15
    */
   GPIOC->MODER |= 0b0101010100000000;
   GPIOC->PUPDR |= 0xAA;
}

void TIM15_IRQHandler(){
    TIM15->SR &= ~TIM_SR_UIF;
    for(int i = 0; i< FIELD_WIDTH;i++){
        for(int j=0; j<FIELD_WIDTH;j++){
            square_set(i,j,player_1, player_2);
        }
    }
}

void update_grid() {
    for(int i = 0; i< FIELD_WIDTH;i++){
        for(int j=0; j<FIELD_WIDTH;j++){
            square_set(i,j,player_1, player_2);
        }
    }
    if(*game_status == 0) {
        draw_ship_tbp(player_1);
    }
    if(*game_status >= 3) {
        display_stats();
    }
}

void setup_tim15(){
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->PSC = 48000-1; 
    TIM15->ARR = 1000-1;
    TIM15->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= 1<< TIM15_IRQn;
    TIM15->CR1 |= TIM_CR1_CEN;
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
    if(game_status == 3) LCD_DrawString(0, 240, WHITE, BLACK, "Player One Wins", 16, 1);
    if(game_status == 4) LCD_DrawString(0, 240, WHITE, BLACK, "Player Two Wins", 16, 1);
    if(game_status == 5) LCD_DrawString(0, 240, WHITE, BLACK, "Tie Game", 16, 1);
    
    char bugger[50];
    sprintf(bugger, "Player One Accuracy: %d%%", (100 * player_1->total_hits)/player_1->total_shots);
    LCD_DrawString(0, 256, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Player One Max Hit Streak: %d", player_1->max_hit_streak);
    LCD_DrawString(0, 272, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Player One Max Miss Streak: %d", player_1->max_miss_streak);
    LCD_DrawString(0, 288, WHITE, BLACK, bugger, 16, 1);

    sprintf(bugger, "Number of rounds: %d", bs_users.num_rounds);
    LCD_DrawString(0, 304, WHITE, BLACK, bugger, 16, 1);

}

void setup_serial(void)
{
    RCC->AHBENR |= 0x00180000;
    GPIOC->MODER  |= 0x02000000;
    GPIOC->AFR[1] |= 0x00020000;
    GPIOD->MODER  |= 0x00000020;
    GPIOD->AFR[0] |= 0x00000200;
    RCC->APB1ENR |= 0x00100000;
    USART5->CR1 &= ~0x00000001;
    USART5->CR1 |= 0x00008000;
    USART5->BRR = 0x340;
    USART5->CR1 |= 0x0000000c;
    USART5->CR1 |= 0x00000001;
}




int __io_putchar(int c) {
    if(c == '\n') {
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';    
    }

    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}
