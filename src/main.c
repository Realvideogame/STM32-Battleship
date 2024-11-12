/**
  ******************************************************************************
  * file    main.c
  * author  Team 39
  * date    Nov 2024
  * brief   Battleship primary main
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "drawing_utility.c"
#include <stdint.h>
#include "primary_setup.h"


int main() {

    /**********************
   Critical: We do not currently distinguish displays or have the means to send to display 2, which has no main yet
   * **************************************/
    // Setups function calls
    internal_clock();

    // Setting up player strcutures
    player_1.x = 0;
    player_2.x = 0;
    player_1.y = 0;
    player_2.y = 0;
    for(int i = 0; i < FIELD_HEIGHT; i++) {
      for(int j = 0; j < FIELD_WIDTH; j++) {
        player_1.field[i][j] = 0;
        player_2.field[i][j] = 0;
      }
    }

    // Battleship
    // phase 1 - placeing ships
    // 120 sec total
    //tbd: find correct rotation
    setup_buttons();
    setup_dma();
    init_master();
    LCD_Setup();
    LCD_direction(1);
    mapgen(); //display empty map to main
    //secondary should also call mapgen
    start_timer(120);
    while(timer_set && ((player_1.ship_index < NUM_SHIPS) || (player_2.ship_index < NUM_SHIPS))) { 

      move_cursor(&player_1); // moves player 1's cursor as needed
      place_ship(&player_1); // place or rotates player 1 ship
      player_1.input = 0;

      move_cursor(&player_2); // moves player 2's cursor as needed
      place_ship(&player_2); // place or rotates player 2 ship
      player_1.input = 0;
    }
  //pwm now needed
  setup_led_array();
    // phase 2 - turn by turn game
    // 60 sec a turn (FOR NOW)
    int winner = 0;
    while (game_status < 3) {
      while(game_status == 1 && attack_turn(&player_1, &player_2) == 1) { // Player 1 Turn
        if(check_player_status(&player_2)) {
          // player 2 has no remaining ships
          winner += 1;
        }
      }
    
      game_status += 1; // setting to next turn
      while((game_status == 2 || game_status == 3) && attack_turn(&player_2, &player_1) == 1) {
        if(check_player_status(&player_1)) {
          // player 1 has no remaining ships
          winner += 2;
        }
      }

      if(winner) {
        game_status = winner + 2; // set game status with the winner
      }
      else game_status = 1; // reset to player one's turn
    }

    // phase 3 - post game stats
    
}
/*
move_cursor:
  Moves the player's cursor cordinates based on the player's input
  Can be used for multiple phases
*/
int move_cursor(player* p) {
  switch (p->input & 0b00111111) {
    case 0b100000: // Move Cursor Up
      if (p->y != 0) p->y -= 1;
      square_clear(p->x, (p->y)-1,p);
      draw_cursor(p->x,p->y);
      break;
    case 0b010000: // Move Cursor Down
      if(p->y < (FIELD_HEIGHT - (1-p->rotation) * ship_size[p->ship_index])) p->y += 1;
      square_clear(p->x, (p->y)+1,p);
      draw_cursor(p->x,p->y);
      break;
    case 0b001000: // Move Cursor Left
      if(p->x != 0) p->x -= 1;
      square_clear((p->x)+1, p->y,p);
      draw_cursor(p->x,p->y);
      break;
    case 0b000100: // Move Cursor Right
      if(p->x < (FIELD_WIDTH - (p->rotation)*ship_size[p->ship_index])) p->x += 1;
      square_clear((p->x)-1, p->y,p);
      draw_cursor(p->x,p->y);
      break;
  }
  return 0;
}

/*
place_ship:
  Will rotate or place (or do nothing) based on the player's current input
  For use during ship placement
*/
int place_ship(player* p) {
  if ((p->input & 0b00111111) == 0b10) { // rotating
    // TODO: make it so the x and y cordinates are not reset
    p->x = 0;
    p->y = 0;
    p->rotation = 1 - p->rotation;
  }
  else if ((p->input & 0b00111111) == 0b01){ // placing ship
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
  }
  return 0;
}

int attack_turn(player* attacker, player* defender) {
  start_timer(60); // 60 Second Turn Timer
  int have_shot = 0; // whether or not the attacker has sucsessfully fired (>0), and whether it was a hit(1) or not(2)
  while(timer_set && !have_shot) {
    move_cursor(attacker); // registering movement from the attacker
    if(attacker->input & 1) { // firing
      if(defender->field[attacker->x][attacker->y] >= 3) { // location already shot
        // Possibly indicate invalid hit with LEDs
      }
      else{
        if (defender->field[attacker->x][attacker->y] == 0) have_shot = 1; // hit
        else have_shot = 2; // miss
        defender->field[attacker->x][attacker->y] += 2; // Turns empty to missed and ship to hit
      }
    }
  }
  return have_shot;
}

int check_player_status(player* p) {
  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++) {
      if(p->field[i][j] == 1) {
        return 0; // player still has ship components
      }
    }
  }
  return 1; // player has no ships remaining
}
