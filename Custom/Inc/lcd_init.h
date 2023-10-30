#ifndef _LCD_INIT_H_
#define _LCD_INIT_H_

#include "sys.h"

#define LCD_BLK_PIN GPIO_PIN_2
#define LCD_BLK_PORT GPIOA
#define LCD_BLK_PORT_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

#define LCD_CS_PIN GPIO_PIN_4
#define LCD_CS_PORT GPIOE
#define LCD_CS_PORT_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

#define LCD_SDA_PIN GPIO_PIN_6
#define LCD_SDA_PORT GPIOE
#define LCD_SDA_PORT_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

#define LCD_SCL_PIN GPIO_PIN_2
#define LCD_SCL_PORT GPIOE
#define LCD_SCL_PORT_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()

#define PIN_OUT(PORT, PIN, STATUS) ((PORT)->BSRR = (STATUS) ? (PIN) : (uint32_t)(PIN) << 16)

#define LCD_BLK(x) PIN_OUT(LCD_BLK_PORT, LCD_BLK_PIN, x) // LCD背光
#define LCD_CS(x) PIN_OUT(LCD_CS_PORT, LCD_CS_PIN, x)
#define LCD_SCL(x) PIN_OUT(LCD_SCL_PORT, LCD_SCL_PIN, x)
#define LCD_SDA(x) PIN_OUT(LCD_SDA_PORT, LCD_SDA_PIN, x)

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

void LCD_Init(uint32_t frambuffer);
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void LCD_Color_Fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height,uint32_t *color);
void LCD_Color_Fill_IT(uint16_t x, uint16_t y, uint16_t width, uint16_t height,uint32_t *color);
void LCD_SetFb_IT(uint32_t fb);

#endif
