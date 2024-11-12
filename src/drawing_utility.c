#include "lcd.h"
#include <primary_setup.h>
/*
void LCD_Setup(void);
void LCD_Init(void (*reset)(int), void (*select)(int), void (*reg_select)(int));
void LCD_Clear(u16 Color);
void LCD_DrawPoint(u16 x,u16 y,u16 c);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 c);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 c);
void LCD_DrawFillRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 c);
void LCD_Circle(u16 xc, u16 yc, u16 r, u16 fill, u16 c);
void LCD_DrawTriangle(u16 x0,u16 y0, u16 x1,u16 y1, u16 x2,u16 y2, u16 c);
void LCD_DrawFillTriangle(u16 x0,u16 y0, u16 x1,u16 y1, u16 x2,u16 y2, u16 c);
void LCD_DrawChar(u16 x,u16 y,u16 fc, u16 bc, char num, u8 size, u8 mode);
void LCD_DrawString(u16 x,u16 y, u16 fc, u16 bg, const char *p, u8 size, u8 mode);*/

//available drawing functions

//this file will contain functions that use basic lcd functions to generate a battleship map and update the battleship map

//must be called on both micros
void mapgen(void){
    //240 by 320 pixel map
    LCD_DrawFillRectangle(0,0,240,320,LIGHTBLUE);
    //3 pixel wide line
    //short axis
    for(int i = 0; i < 240/9; i+=240/9){ //integer division
        LCD_DrawFillRectangle(i,0,i+3,320, DARKBLUE);
    }
    //long axis
    for(int i = 0; i <= 320/9; i+=320/9){
        LCD_DrawFillRectangle(i,0,i+3,240, DARKBLUE);
    }
}

void square_clear(u8 x_coord, u8 y_coord, player* p){
    if(p->field[p->y][p->x] == 0){
    //clear space
    LCD_DrawFillRectangle(x_coord-20,y_coord-20,x_coord,y_coord,LIGHTBLUE);
    } else if(p->field[p->y][p->x] == 1){
        //circle for ship
    }else if(p->field[p->y][p->x] == 2){
        //blue X for miss
    }else if(p->field[p->y][p->x] == 3){
        //Orang Circle for dead ship bit
    }
}
draw_cursor(u8 x, u8 y){
    //a + of 2 lines, should probably be drawn on top of the ship piece
    LCD_DrawLine(x,y-13,x-26,y-13,DARKBLUE);
    LCD_DrawLine(x+13,y,x-13,y-26,DARKBLUE);
}
//hard function!
//void draw_ship(u8 rotation, )