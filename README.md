# Battleship Game Implementation with STM32F091 Microcontrollers  

This project was developed as part of [ECE 36200 - Microcontroller Systems and Interfacing](https://engineering.purdue.edu/ECE/Academics/Undergraduates/UGO/CourseInfo?courseid=278&show=true) at Purdue University. The implementation includes elements of both hardware and software design, leveraging the STM32F091 microcontroller to recreate the classic game of Battleship.  

## Acknowledgments  
- **Niraj Menon**: Lab coordinator for ECE 36200, contributed to the LCD implementation code and designed the development board used in this project. Some functions relating to input are taken from labwork for ECE 36200 as well.  

## Overview  
The game is implemented using two STM32F091 microcontrollers. Each player uses a TFT LCD to display game states. The game includes three distinct phases:  
1. **Placement Phase**: Players position their ships on a 9x9 grid. Ship rotation is implemented.
   - Ship sizes are 5,4,3,2,2.
   - Both players can place their ships at the same time.
   - This phase ends when both players have placed all ships.
2. **Gameplay Phase**: Players take turns attacking the opponent's grid.  
   - If attacking, the player sees their previous attacks. The other player sees their own grid, with enemy attacks overlaid.  
   - Turn time is indicated by an RGB LED controlled through PWM:  
     - The green LED decreases in brightness as time runs out.  
     - When nearly out of time, the LED flashes red.
   - The game will end when a player's ships are all sunk. If the first player to go wins, the second player is allowed a redemption turn, which can result in a tie.
3. **Post-Game Phase**: Displays game statistics for players to review.
   - Number of Turns
   - Player accuracy
   - Greatest Hit/Miss Streaks 

## Features  
- **Keypad User Input**:  
  Players can use the keys on a 4x4 keypad to control their cursor, fire, or rotate ships during their turn.
- **TFT LCD Integration**:  
  Each player's grid is displayed on a dedicated LCD screen. Communication with the LCD is handled via SPI.  
- **Interboard Communication**:  
  SPI is also used for communication between the two microcontrollers to synchronize game states.  
- **Turn Timer with RGB LED**:  
  A visual timer implemented with PWM, dynamically indicating remaining time with color changes.


## Hardware  
- **Microcontroller**: [STM32F091RC](https://www.st.com/en/microcontrollers-microprocessors/stm32f091rc.html)  
- **TFT LCD Screens**: Used to display the game grids.  
- **RGB LED**: Provides real-time feedback for turn duration.
- **4x4 Keypad**: Allows for user input
- **Power Supply**: Game be powered by a generic 5V wall plug.

## How to Play    
1. Each player places their ships in any valid configuration.
2. Take turns attacking the opponent's grid while managing your time with the RGB LED timer.  
3. At the end of the game, review the displayed statistics and see who won.  

## Future Plans  
- Finish PCB implementation so the project can exist as a standalone board.  

---

Developed with a focus on embedded systems programming and hardware/software integration.  
