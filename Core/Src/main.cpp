#include "cmsis_os.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "sys.h"
#include "lcd.h"
#include "malloc.h"
#include "sdmmc.h"

void MX_FREERTOS_Init(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  SCB_EnableICache();  // 打开ICache
  SCB_EnableDCache();  // 打开DCache
  SCB->CACR |= 1 << 2; // 强制D-Cache透写

  HAL_Init();
  SystemClock_Config();
  delay_init();
  SDMMC1_SD_Init();
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  mallco_dev.init(0);

  // MX_USB_DEVICE_Init();
  // while (1)
  // {
  //   HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
  //   HAL_Delay(500);
  // }

  MX_FREERTOS_Init();

  osKernelStart();

  while (1)
  {
  }
}

extern "C"
{
  /**
   * @brief  Period elapsed callback in non blocking mode
   * @note   This function is called  when TIM13 interrupt took place, inside
   * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
   * a global variable "uwTick" used as application time base.
   * @param  htim : TIM handle
   * @retval None
   */
  void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
  {
    if (htim->Instance == TIM13)
    {
      HAL_IncTick();
    }
  }
}
