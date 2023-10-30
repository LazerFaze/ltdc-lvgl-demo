#include "tim.h"
#include "lcd_init.h"

static TIM_HandleTypeDef lcd_brightness_handle;		//LCD背光控制定时器句柄

void LCD_Brightness_Timer_Init(void)
{
	TIM_OC_InitTypeDef TIM_OC_InitStructure = {0};
	
	lcd_brightness_handle.Instance = LCD_BRIGHTNESS_TIMER;
	lcd_brightness_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;		
	lcd_brightness_handle.Init.CounterMode = TIM_COUNTERMODE_UP;		//向上计数
	lcd_brightness_handle.Init.Period = 100 - 1;		//500Hz
	lcd_brightness_handle.Init.Prescaler = 5600 - 1;		//5600分频，50KHz
	lcd_brightness_handle.Init.RepetitionCounter = 0;
	
	HAL_TIM_PWM_Init(&lcd_brightness_handle);		//初始化定时器
	
	TIM_OC_InitStructure.OCMode = TIM_OCMODE_PWM1;
	TIM_OC_InitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStructure.Pulse = 50;		//占空比50%
	HAL_TIM_PWM_ConfigChannel(&lcd_brightness_handle, &TIM_OC_InitStructure, LCD_BRIGHTNESS_TIMER_CHANNEL);
	
	HAL_TIM_PWM_Start(&lcd_brightness_handle, LCD_BRIGHTNESS_TIMER_CHANNEL);				//开启PWM输出
}

//设置占空比
void LCD_BrightnessSetPulse(uint32_t pulse)
{
	lcd_brightness_handle.Instance->CCR3 = pulse;
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	if(htim->Instance == LCD_BRIGHTNESS_TIMER)
	{
		LCD_BRIGHTNESS_TIMER_CLK_ENABLE();		//开启定时器的时钟
		LCD_BLK_PORT_CLK_ENABLE();
		
		GPIO_InitStructure.Alternate = GPIO_AF1_TIM2;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pin = LCD_BLK_PIN;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(LCD_BLK_PORT, &GPIO_InitStructure);
	}
}
