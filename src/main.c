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

#define FIELD_WIDTH 9
#define FIELD_HEIGHT 12
#define NUM_SHIPS 5

void internal_clock();

// battleship function
// TODO: move to header
int move_cursor(player*);
int place_ship(player* p);

int8_t ship_size[NUM_SHIPS] = {5, 4, 3, 2, 2};



typedef struct _player{
  int8_t x; // Player's cursor x-cord
  int8_t y; // Player's cursor y-cord

  int8_t field[FIELD_HEIGHT][FIELD_WIDTH];// Fields of each player's ships
  // 0 - Empty
  // 1 - Ship
  // 2 - missed
  // 3 - hit

  int8_t input; // player's input stored as XXABCDEF
  // A - Up
  // B - Down
  // C - Left
  // D - Right
  // E - rotate
  // F - fire / place

  // ship placement variables
  int8_t ship_index; // current ship being placed
  int8_t rotation; // Ship rotation (0 - vert, 1 - horz)
  

} player;

player player_1;
player player_2;

int8_t timer_set;
int start_timer(int); // waits x secounds, should set a global variable to 1 and have a timer go for x sec, trigering an interupt after x sec which sets the global variable to 0

int main() {
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
    start_timer(120);
    while(timer_set && ((player_1.ship_index < NUM_SHIPS) || (player_2.ship_index < NUM_SHIPS))) { 

      move_cursor(&player_1); // moves player 1's cursor as needed
      place_ship(&player_1); // place or rotates player 1 ship
      player_1.input = 0;

      move_cursor(&player_2); // moves player 2's cursor as needed
      place_ship(&player_2); // place or rotates player 2 ship
      player_1.input = 0;
    }

    // phase 2 - turn by turn game
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
      break;
    case 0b010000: // Move Cursor Down
      if(p->y < (FIELD_HEIGHT - (1-p->rotation) * ship_size[p->ship_index])) p->y += 1;
      break;
    case 0b001000: // Move Cursor Left
      if(p->x != 0) p->x -= 1;
      break;
    case 0b000100: // Move Cursor Right
      if(p->x < (FIELD_WIDTH - (p->rotation)*ship_size[p->ship_index])) p->x += 1;
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
