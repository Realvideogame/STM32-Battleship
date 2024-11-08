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

#define FIELD_SIZE 8

void internal_clock();

int8_t p1_field[FIELD_SIZE][FIELD_SIZE] = {0};
int8_t p2_field[FIELD_SIZE][FIELD_SIZE] = {0};

int main() {
    internal_clock();
    
    // Setups function calls

    // Battleship
    // phase 1 - placeing ships

    // phase 2 - turn by turn game

    // phase 3 - post game stats
    
}