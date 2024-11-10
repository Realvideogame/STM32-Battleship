/**
  ******************************************************************************
  * file    main.c
  * author  Team 39
  * date    Nov 2024
  * brief   Battleship primary main
  ******************************************************************************
*/

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