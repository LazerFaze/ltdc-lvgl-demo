#ifndef _SYS_H_
#define _SYS_H_

#include "stdio.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

void SystemClock_Config(void);
void delay_init(void);
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);


#ifdef __cplusplus
}
#endif



#endif
