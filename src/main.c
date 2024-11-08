/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An, Niraj Menon
  * @date    Feb 7, 2024
  * @brief   ECE 362 Lab 7 student template
  ******************************************************************************
*/

/*******************************************************************************/

// Fill out your username!  Even though we're not using an autotest, 
// it should be a habit to fill out your username in this field now.
const char* username = "sing1168";

/*******************************************************************************/ 

#include "stm32f0xx.h"
#include <stdint.h>

void internal_clock();

// Uncomment only one of the following to test each step
//#define STEP1
 //#define STEP2
 //#define STEP3
 //#define STEP4
 //#define SHELL

void init_usart5() {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;  
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;  
    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    GPIOC->MODER &= ~(GPIO_MODER_MODER12);         
    GPIOC->MODER |= GPIO_MODER_MODER12_1;          
    GPIOC->AFR[1] &= ~(GPIO_AFRH_AFSEL12);         
    GPIOC->AFR[1] |= (2 << GPIO_AFRH_AFSEL12_Pos); 
    GPIOD->MODER &= ~(GPIO_MODER_MODER2);      
    GPIOD->MODER |= GPIO_MODER_MODER2_1;         
    GPIOD->AFR[0] &= ~(GPIO_AFRL_AFSEL2);          
    GPIOD->AFR[0] |= (2 << GPIO_AFRL_AFSEL2_Pos); 

    USART5->CR1 &= ~USART_CR1_UE;
    USART5->CR1 &= ~USART_CR1_M0;
    USART5->CR1 &= ~USART_CR1_M1;
    USART5->CR2 &= ~USART_CR2_STOP;
    USART5->CR1 &= ~USART_CR1_PCE;
    USART5->CR1 &= ~USART_CR1_OVER8;
    USART5->BRR = 0x1A1;
    USART5->CR1 |= USART_CR1_TE;
    USART5->CR1 |= USART_CR1_RE;

    USART5->CR1 |= USART_CR1_UE;

    while (!(USART5->ISR & USART_ISR_TEACK)); 
    while (!(USART5->ISR & USART_ISR_REACK)); 
}

#ifdef STEP1
int main(void){
    internal_clock();
    init_usart5();
    for(;;) {
        while (!(USART5->ISR & USART_ISR_RXNE)) { }
        char c = USART5->RDR;
        while(!(USART5->ISR & USART_ISR_TXE)) { }
        USART5->TDR = c;
    }
}
#endif

#ifdef STEP2
#include <stdio.h>

// TODO Resolve the echo and carriage-return problem

int __io_putchar(int c) {
    // TODO
    if (c == '\n') {
        while (!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while (!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    while (!(USART5->ISR & USART_ISR_RXNE));
    char c = USART5->RDR;
    // TODO
    if(c == '\r'){
        c = '\n';
    }
    __io_putchar(c);
    return c;
}

int main() {
    internal_clock();
    init_usart5();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: ");
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n");
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

#ifdef STEP3
#include <stdio.h>
#include "fifo.h"
#include "tty.h"

int __io_putchar(int c) {
    // TODO
    if(c == '\n'){
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    // TODO
    int rtv = line_buffer_getchar();
    return rtv;
}

int main() {
    internal_clock();
    init_usart5();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: ");
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n");
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

#ifdef STEP4

#include <stdio.h>
#include "fifo.h"
#include "tty.h"

// TODO DMA data structures
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void enable_tty_interrupt(void) {
    // TODO
    USART5->CR1 |= USART_CR1_RXNEIE;

    USART5->CR3 |= USART_CR3_DMAR;
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->CSELR |= DMA2_CSELR_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // First make sure DMA is turned off

    // The DMA channel 2 configuration goes here
    DMA2_Channel2->CMAR = (uint32_t) serfifo;
    DMA2_Channel2->CPAR = (uint32_t)&(USART5->RDR);
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR |= DMA_CCR_MINC;
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE;
    DMA2_Channel2->CCR |= DMA_CCR_CIRC;
    DMA2_Channel2->CCR |= DMA_CCR_PL_Msk;
    DMA2_Channel2->CCR |= DMA_CCR_EN;
    
    NVIC_EnableIRQ(USART3_8_IRQn);

}

// Works like line_buffer_getchar(), but does not check or clear ORE nor wait on new characters in USART
char interrupt_getchar() {
    // TODO
    while(!(fifo_newline(&input_fifo))){
        asm volatile ("wfi");
    }
    return fifo_remove(&input_fifo);

}

int __io_putchar(int c) {
    // TODO copy from STEP2
     if(c == '\n'){
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    // TODO Use interrupt_getchar() instead of line_buffer_getchar()
    return interrupt_getchar();
}

// TODO Copy the content for the USART5 ISR here
// TODO Remember to look up for the proper name of the ISR function

void USART3_8_IRQHandler(void) {
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
        if (!fifo_full(&input_fifo))
            insert_echo_char(serfifo[seroffset]);
        seroffset = (seroffset + 1) % sizeof serfifo;
    }
}

int main() {
    internal_clock();
    init_usart5();
    enable_tty_interrupt();

    setbuf(stdin,0); // These turn off buffering; more efficient, but makes it hard to explain why first 1023 characters not dispalyed
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: "); // Types name but shouldn't echo the characters; USE CTRL-J to finish
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n"); // After, will type TWO instead of ONE
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

#ifdef SHELL
#include <stdio.h>
#include "fifo.h"
#include "tty.h"
#include "commands.h"
#include "ff.h"

// TODO DMA data structures
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void enable_tty_interrupt(void) {
    // TODO
    USART5->CR1 |= USART_CR1_RXNEIE;

    USART5->CR3 |= USART_CR3_DMAR;
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->CSELR |= DMA2_CSELR_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // First make sure DMA is turned off

    // The DMA channel 2 configuration goes here
    DMA2_Channel2->CMAR = (uint32_t) serfifo;
    DMA2_Channel2->CPAR = (uint32_t)&(USART5->RDR);
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR |= DMA_CCR_MINC;
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE;
    DMA2_Channel2->CCR |= DMA_CCR_CIRC;
    DMA2_Channel2->CCR |= DMA_CCR_PL_Msk;
    DMA2_Channel2->CCR |= DMA_CCR_EN;
    
    NVIC_EnableIRQ(USART3_8_IRQn);

}
char interrupt_getchar() {
    // TODO
    while(!(fifo_newline(&input_fifo))){
        asm volatile ("wfi");
    }
    return fifo_remove(&input_fifo);

}
int __io_putchar(int c) {
    // TODO copy from STEP2
     if(c == '\n'){
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    // TODO Use interrupt_getchar() instead of line_buffer_getchar()
    return interrupt_getchar();
}
void USART3_8_IRQHandler(void) {
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
        if (!fifo_full(&input_fifo))
            insert_echo_char(serfifo[seroffset]);
        seroffset = (seroffset + 1) % sizeof serfifo;
    }
}

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
    GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER11 | GPIO_MODER_MODER14);  // Clear current mode
    GPIOB->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER14_0);
    init_spi1_slow();
    sdcard_io_high_speed();
}
int main() {
    internal_clock();
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    command_shell();
}
#endif