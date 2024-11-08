#include <primary_setup.h>
#include <stm32f091xc.h>
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
void EXTI0_1IRQHANDLER(){

}
void EXTI2_3_IRQHandler(){

}
void EXTI4_15_IRQHandler(){

}