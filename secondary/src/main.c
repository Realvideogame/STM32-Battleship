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
#include "string.h"


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
    // match_users = bs_users;

    ship_size[0] = 5;
    ship_size[1] = 4;
    ship_size[2] = 3;
    ship_size[3] = 2;
    ship_size[4] = 2;
    
    ship_size[5] = 1;

    while(1) {
      // if(memcmp(&bs_users, &match_users, sizeof(users))) {
      //   match_users = bs_users;
      //   update_grid();
      // }
      for(int j = 0; j < 100000; j++);
      update_grid();
    }
}
/*
move_cursor:
  Moves the player's cursor cordinates based on the player's input
  Can be used for multiple phases
*/