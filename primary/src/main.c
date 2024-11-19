/**
  ******************************************************************************
  * file    main.c
  * author  Team 39
  * date    Nov 2024
  * brief   Battleship primary main
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "stdio.h"
#include <stdint.h>
#include "primary_setup.h"
#include "lcd.h"

extern int32_t time_remaining;

int main() {

    /**********************
   Critical: We do not currently distinguish displays or have the means to send to display 2, which has no main yet
   * **************************************/
    // Setups function calls
    internal_clock();
    player_1 = &bs_users.p1;
    player_2 = &bs_users.p2;
    game_status = &bs_users.status;
    bs_users.num_rounds = 0;
    setup_dma();
    init_master();
    LCD_Setup();
    setup_turn_timer();
    mapgen(); //display empty map to main
    enable_gpioC();
    setup_tim14();
    // setup_serial();

    ship_size[0] = 5;
    ship_size[1] = 4;
    ship_size[2] = 3;
    ship_size[3] = 2;
    ship_size[4] = 2;
    // Setting up player strcutures
    player_1->x = 0;
    player_2->x = 0;
    player_1->y = 0;
    player_2->y = 0;
    player_1->input = 0;
    player_2->input = 0;

    player_1->player_num = 1;
    player_2->player_num = 2;

    for(int i = 0; i < FIELD_WIDTH; i++) {
      for(int j = 0; j < FIELD_WIDTH; j++) {
        player_1->field[i][j] = 0;
        player_2->field[i][j] = 0;
      }
    }

    // Battleship
    // phase 1 - placeing ships
    // 300 sec total
    //tbd: find correct rotation
    //secondary should also call mapgen

    update_grid();
    *game_status = 0;
    while(((player_1->ship_index < NUM_SHIPS) || (player_2->ship_index < NUM_SHIPS))) { 

      if (player_1->ship_index < NUM_SHIPS) {
        move_cursor(player_1); // moves player 1's cursor as needed
        place_ship(player_1); // place or rotates player 1 ship
      }

      if (player_2->ship_index < NUM_SHIPS) {
        move_cursor(player_2); // moves player 2's cursor as needed
        place_ship(player_2); // place or rotates player 2 ship
      }
    }
    //pwm now needed
    
    
    // phase 2 - turn by turn game
    // 60 sec a turn (FOR NOW)
    int winner = 0;
    *game_status = 1;
    update_grid();
    setup_led_array();
    while (*game_status < 3) {
      bs_users.num_rounds++;
      while(*game_status == 1 && (attack_turn(player_1, player_2) == 1)) { // Player 1 Turn
        if(check_player_status(player_2)) {
          // player 2 has no remaining ships
          winner += 1;
          break;
        }
      }
    
      *game_status = 2; // setting to next turn
      update_grid();
      while((*game_status == 2) && (attack_turn(player_2, player_1) == 1)) {
        if(check_player_status(player_1)) {
          // player 1 has no remaining ships
          winner += 2;
          break;
        }
      }

      if(winner) {
        *game_status = winner + 2; // set game status with the winner
      }
      else *game_status = 1; // reset to player one's turn
      update_grid();
    }

    // phase 3 - post game stats
    update_grid();
    display_stats();
    
}
/*
move_cursor:
  Moves the player's cursor cordinates based on the player's input
  Can be used for multiple phases
*/
int move_cursor(player* p) {
  int size = 0;
  if(*game_status == 0) size = ship_size[p->ship_index]-1;
  
  switch (p->input & 0b00111111) {
    case 0b100000: // Move Cursor Up
      if (p->y != 0) {
        p->y -= 1;
        if(p->player_num == 1) update_grid();
      }
      break;
    case 0b010000: // Move Cursor Down
      if(p->y < FIELD_WIDTH - 1 - (1-p->rotation)*(size)) {
        p->y += 1;
        if(p->player_num == 1) update_grid();
      }
      break;
    case 0b001000: // Move Cursor Left
      if(p->x != 0) {
        p->x -= 1;
        if(p->player_num == 1) update_grid();
      }
      break;
    case 0b000100: // Move Cursor Right
      if(p->x < FIELD_WIDTH - 1 - (p->rotation)*(size)) {
        p->x += 1;
        if(p->player_num == 1) update_grid();
      }
      break;
  }
  p->input = 0;
  return 0;
}

/*
place_ship:
  Will rotate or place (or do nothing) based on the player's current input
  For use during ship placement
*/
int place_ship(player* p) {
  if ((p->input & 0b00111111) == 0b10) { // rotating
    p->input = 0;
    p->x = 0;
    p->y = 0;
    p->rotation = 1 - p->rotation;
    if(p->player_num == 1) update_grid();
  }
  else if ((p->input & 0b00111111) == 0b01){ // placing ship
    p->input = 0;
    if(p->rotation) { // placing horizontal ship
      for(int i = 0; i < ship_size[p->ship_index]; i++) {
        if(p->field[p->y][p->x+i] != 0) return 1; // Fail if any of the squares are not empty
      }
      for(int i = 0; i < ship_size[p->ship_index]; i++) {
        p->field[p->y][p->x+i] = 1;
      }
    }
    else { // placing vertical ship
      for(int i = 0; i < ship_size[p->ship_index]; i++) {
        if(p->field[p->y+i][p->x] != 0) return 1; // Fail if any of the squares are not empty
      }
      for(int i = 0; i < ship_size[p->ship_index]; i++) {
        p->field[p->y+i][p->x] = 1;
      }
    }
    // Ship sucsessfully placed
    p->ship_index++;
    p->x=0;
    p->y=0;
    p->rotation=0;
    if(p->player_num == 1) update_grid();
  }
  return 0;
}

int attack_turn(player* attacker, player* defender) {
  start_new_round_LED();
  int have_shot = 0; // whether or not the attacker has sucsessfully fired (>0), and whether it was a hit(1) or not(2)
  
  while(time_remaining != -1 && !have_shot) {
    move_cursor(attacker); // registering movement from the attacker
    
    if(attacker->input & 1) { // firing
      if(defender->field[attacker->y][attacker->x] >= 2) { // location already shot
        // Possibly indicate invalid hit with LEDs
      }
      else{
        if (defender->field[attacker->y][attacker->x] == 1) { // hit
          have_shot = 1;
          attacker->total_shots += 1;
          attacker->total_hits += 1;
          
          attacker->cur_hit_streak += 1;
          attacker->cur_miss_streak = 0;
          if(attacker->cur_hit_streak > attacker->max_hit_streak) attacker->max_hit_streak = attacker->cur_hit_streak;
        } 
        else {
          have_shot = 2; // miss
          attacker->total_shots += 1;

          attacker->cur_hit_streak = 0;
          attacker->cur_miss_streak += 1;
          if(attacker->cur_miss_streak > attacker->max_miss_streak) attacker->max_miss_streak = attacker->cur_miss_streak;
        }
        defender->field[attacker->y][attacker->x] += 2; // Turns empty to missed and ship to hit
        update_grid();
      }
    }
  }
  stop_LED();
  return have_shot;
}

int check_player_status(player* p) {
  for (int i = 0; i < FIELD_WIDTH; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++) {
      if(p->field[i][j] == 1) {
        return 0; // player still has ship components
      }
    }
  }
  return 1; // player has no ships remaining
}
