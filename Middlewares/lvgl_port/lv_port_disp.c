/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lcd.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES 320
#define MY_DISP_VER_RES 240
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static void disp_wait_flush(lv_disp_drv_t *disp_drv);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_color_t color_buf1[MY_DISP_HOR_RES * MY_DISP_VER_RES] __section(".axi_ram"); // 显存1定义
static lv_color_t color_buf2[MY_DISP_HOR_RES * MY_DISP_VER_RES] __section(".axi_ram"); // 显存2定义

static xSemaphoreHandle dispFlushSem; // 刷新等待信号量
static SemaphoreHandle_t lvglMutex;   // lvgl对象访问互斥量
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
  disp_init();

  static lv_disp_draw_buf_t draw_buf_dsc;
  lv_disp_draw_buf_init(&draw_buf_dsc, color_buf1, color_buf2, MY_DISP_HOR_RES * MY_DISP_VER_RES); /*Initialize the display buffer*/

  static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
  lv_disp_drv_init(&disp_drv);   /*Basic initialization*/

  disp_drv.hor_res = MY_DISP_HOR_RES;
  disp_drv.ver_res = MY_DISP_VER_RES;
  disp_drv.flush_cb = disp_flush;
  disp_drv.wait_cb = disp_wait_flush;
  disp_drv.draw_buf = &draw_buf_dsc;
  disp_drv.full_refresh = 1; // 整个屏幕重绘
  lv_disp_drv_register(&disp_drv);

  dispFlushSem = xSemaphoreCreateBinary();
  lvglMutex = xSemaphoreCreateMutex(); // 创建互斥量
}

void lv_mutex_lock(void)
{
  xSemaphoreTake(lvglMutex, portMAX_DELAY);
}

void lv_mutex_unlock(void)
{
  xSemaphoreGive(lvglMutex);
}

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
  /*You code here*/
  LCD_Init((uint32_t)color_buf2); // LCD初始化
  LCD_Clear(0xffff);
}

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
  LCD_SetFb_IT((uint32_t)color_p); // 更改LCD前缓冲区，并开启中断
}

static void disp_wait_flush(lv_disp_drv_t *disp_drv)
{
  xSemaphoreTake(dispFlushSem, portMAX_DELAY); // 等待刷新完毕信号量
  lv_disp_flush_ready(disp_drv);
}

// LTDC 行中断触发，前缓冲区刷新完毕
void LCD_SetFb_Cplt(void)
{
  BaseType_t pxHigherPriorityTaskWoken;

  xSemaphoreGiveFromISR(dispFlushSem, &pxHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
