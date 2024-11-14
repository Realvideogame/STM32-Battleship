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

void mapgen(void);
void square_set(uint8_t, uint8_t, player*);



void internal_clock();

// battleship functions
int move_cursor(player*);
int place_ship(player* p);
int attack_turn(player* attacker, player* defender);
int check_player_status(player* p);

int8_t ship_size[NUM_SHIPS];


player player_1;
player player_2;
int8_t game_status; // 0 - placing ship phase, 1 - player 1's turn, 2 - player 2's turn, 3 - player 1 won, 4 - player 2 won, 5 - Tied Game

int8_t timer_set;
void start_timer(int); // waits x secounds, should set a global variable to 1 and have a timer go for x sec, trigering an interupt after x sec which sets the global variable to 0

uint8_t col; // the column being scanned

void drive_column(int);   // energize one of the column outputs
int  read_rows();         // read the four row inputs
void update_history(int col, int rows); // record the buttons of the driven column
char get_key_event(void); // wait for a button event (press or release)
char get_keypress(void);  // wait for only a button press event.
float getfloat(void);     // read a floating-point number from keypad
void setup_tim14(void);
void setup_tim15(void);

#endif