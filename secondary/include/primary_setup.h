// Function Protoypes

// Setup-functions
//pcb sexton

#ifndef PRIMARY_SETUP

#define PRIMARY_SETUP

#include <stdint.h>
void setup_led_array(void); // cecilie
void setup_turn_timer(void); // bug
void setup_lcd(void); //done
void setup_dma(void);
void setup_interboard_com_primary(void);
void setup_dma(void);
void DMA1_Channel4_5_IRQHandler(void);
void init_master(void);
void init_slave(void);
void TIM7_IRQHandler(void);
void disable_turn_timer(void);
void startPWM(void);

#define FIELD_WIDTH 9
#define FIELD_HEIGHT 12
#define NUM_SHIPS 5

typedef struct _player{
  int8_t player_num;

  int8_t x; // Player's cursor x-cord
  int8_t y; // Player's cursor y-cord

  int8_t field[FIELD_WIDTH][FIELD_WIDTH]; // Fields of each player's ships
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
  
  // Game Stats
  int8_t cur_hit_streak;
  int8_t max_hit_streak;
  int8_t cur_miss_streak;
  int8_t max_miss_streak;
  
  int8_t total_hits;
  int8_t total_shots;

} player;

typedef struct _users {
  player p1;
  player p2;
  int8_t status;
  int8_t num_rounds;
} users;

users bs_users;
player* player_1;
player* player_2;
int8_t* game_status; // 0 - placing ship phase, 1 - player 1's turn, 2 - player 2's turn, 3 - player 1 won, 4 - player 2 won, 5 - Tied Game

int8_t ship_size[NUM_SHIPS+1];

int8_t timer_set;

void internal_clock();

void mapgen(void);
void square_set(uint8_t, uint8_t, player*, player*);
void update_grid();

uint8_t col; // the column being scanned



#endif