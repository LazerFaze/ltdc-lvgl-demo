/**
 * @file lv_port_indev_templ.c
 *
 */

 /*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "gt1151.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/


static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);



/**********************
 *  STATIC VARIABLES
 **********************/
static tp_dev_t *dev;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */
    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/
    GT1151_Init();
		dev = TPGetDevice();		//获取触摸设备

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/


/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  static uint16_t last_x = 0;
	static uint16_t last_y = 0;
	
	if(dev->sta & TP_PRES_DOWN)//触摸按下了
	{
    dev->scan(0);

		last_x = dev->x[0];
		last_y = dev->y[0];	//横屏
		
		data->point.x = last_x;
		data->point.y = last_y;
		
		data->state = LV_INDEV_STATE_PR;
	}
	else
	{			//触摸松开了
		data->point.x = last_x;
		data->point.y = last_y;
		
		data->state = LV_INDEV_STATE_REL;
	}
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
