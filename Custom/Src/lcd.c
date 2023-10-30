#include "lcd.h"



void LCD_Clear(uint16_t color)
{
	LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}



