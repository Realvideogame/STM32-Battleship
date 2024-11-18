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
#include "primary_setup.h"
#include "lcd.h"



int main() {

    /**********************
   Critical: We do not currently distinguish displays or have the means to send to display 2, which has no main yet
   * **************************************/
    // Setups function calls
    internal_clock();
    setup_dma();
    init_slave();
    LCD_Setup();
    mapgen(); //display empty map to main

    player_1 = &(bs_users.p1);
    player_2 = &(bs_users.p2);
    game_status = &(bs_users.status);

    ship_size[0] = 5;
    ship_size[1] = 4;
    ship_size[2] = 3;
    ship_size[3] = 2;
    ship_size[4] = 2;
    
    ship_size[5] = 1;
    while(1) {
      update_grid();
    }
}
/*
move_cursor:
  Moves the player's cursor cordinates based on the player's input
  Can be used for multiple phases
*/