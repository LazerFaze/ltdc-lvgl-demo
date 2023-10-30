#ifndef _TIM_H_
#define _TIM_H_

#include "main.h"


#ifdef __cplusplus
extern "C" {
#endif


#define LCD_BRIGHTNESS_TIMER TIM2
#define LCD_BRIGHTNESS_TIMER_CHANNEL TIM_CHANNEL_3
#define LCD_BRIGHTNESS_TIMER_CLK_ENABLE() __HAL_RCC_TIM2_CLK_ENABLE()


void LCD_Brightness_Timer_Init(void);
void LCD_BrightnessSetPulse(uint32_t pulse);

#ifdef __cplusplus
}
#endif



#endif
