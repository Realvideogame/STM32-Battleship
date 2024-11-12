// Function Protoypes

// Setup-functions
//pcb sexton
void setup_led_array(void); // cecilie
void setup_buttons(void); //tushy
void setup_turn_timer(void); // bug
void setup_lcd(void); //done
void setup_dma(void);
void setup_interboard_com_primary(void);
void start_new_round_LED(void);
void init_master(void);
void init_slave(void);

void mapgen(void);
void square_clear(u8, u8, player*);

#define FIELD_WIDTH 9
#define FIELD_HEIGHT 12
#define NUM_SHIPS 5

void internal_clock();

// battleship functions
int move_cursor(player*);
int place_ship(player* p);
int attack_turn(player* attacker, player* defender);
int check_player_status(player* p);

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
int8_t game_status; // 0 - placing ship phase, 1 - player 1's turn, 2 - player 2's turn, 3 - player 1 won, 4 - player 2 won, 5 - Tied Game

int8_t timer_set;
int start_timer(int); // waits x secounds, should set a global variable to 1 and have a timer go for x sec, trigering an interupt after x sec which sets the global variable to 0

