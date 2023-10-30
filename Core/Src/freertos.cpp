#include "FreeRTOS.h"
#include "task.h"
#include "sys.h"
#include "cmsis_os.h"
#include "usb_device.h"

#include "lvgl.h"
#include "demos/lv_demos.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

// Thread Handle
osThreadId ledTaskHandle;
osThreadId lvglTaskHandle;

void ledTask(const void *argument);
void lvglTask(const void *argument);

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None 
 */
void MX_FREERTOS_Init(void)
{
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();

  /* Create the thread(s) */
  osThreadDef(ledTask, ledTask, osPriorityBelowNormal, 0, 128);
  ledTaskHandle = osThreadCreate(osThread(ledTask), NULL);

  osThreadDef(lvglTask, lvglTask, osPriorityNormal, 0, 4096);
  ledTaskHandle = osThreadCreate(osThread(lvglTask), NULL);
}

/**
 * @brief  lvgl线程，优先级Normal
 */
void lvglTask(const void *argument)
{
  lv_demo_widgets();

  while (1)
  {
    lv_mutex_lock();
    uint32_t tick = lv_task_handler();
    lv_mutex_unlock();

    vTaskDelay(tick);
  }
}

/**
 * @brief  LED线程，优先级BelowNormal
 */
void ledTask(const void *argument)
{
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
    osDelay(500);
  }
}
