#ifndef _GT1151_H_
#define _GT1151_H_

#include "sys.h"	

#define GT_INT_PIN_INDEX 0
#define GT_INT_PIN GPIO_PIN_0
#define GT_INT_PORT GPIOE
#define GT_INT_PORT_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()
#define GT_INT_IRQn EXTI0_IRQn
#define GT_INT_IRQHandler EXTI0_IRQHandler


#define GT_RST_PIN GPIO_PIN_1
#define GT_RST_PORT GPIOE
#define GT_RST_PORT_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()


//IO操作函数	 
#define PIN_IN(PORT, PIN) (!!((PORT)->IDR & (PIN)))
#define PIN_OUT(PORT, PIN, STATUS) ((PORT)->BSRR = (STATUS) ? (PIN) : (uint32_t)(PIN) << 16)
#define PIN_PULL(PORT, PIN_INDEX, PULL) ((PORT)->PUPDR = PULL << ((PIN_INDEX) * 2))

#define GT_RST(x)    		PIN_OUT(GT_RST_PORT, GT_RST_PIN, x)	//GT1151复位引脚
#define GT_INT(x)    		PIN_OUT(GT_INT_PORT, GT_INT_PIN, x)	//GT1151中断引脚
#define GT_INTR()    		PIN_IN(GT_INT_PORT, GT_INT_PIN)		//GT1151中断引脚	
   	
 
//I2C器件地址
#define GT_ADDR 		(0x14 << 1)     	//I2C器件地址1

  
/*  
//GT1151 部分寄存器定义 
#define GT_CTRL_REG 	0X8040   	//GT1151控制寄存器
#define GT_CFGS_REG 	0X8047   	//GT1151配置起始地址寄存器
#define GT_CHECK_REG 	0X80FF   	//GT1151校验和寄存器
#define GT_PID_REG 		0X8140   	//GT1151产品ID寄存器

#define GT_GSTID_REG 	0X814E   	//GT1151当前检测到的触摸情况
#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址  
*/

//GT1151 部分寄存器定义 
#define GT_CTRL_REG 	0X8040   	//GT1151控制寄存器
#define GT_CFGS_REG 	0X8050   	//GT1151配置起始地址寄存器
#define GT_CHECK_REG 	0X813C   	//GT1151校验和寄存器
#define GT_PID_REG 		0X8140   	//GT1151产品ID寄存器

#define GT_GSTID_REG 	0X814E   	//GT1151当前检测到的触摸情况
#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址

#define TP_PRES_DOWN 0x80  //触屏被按下	  
#define TP_CATH_PRES 0x40  //有按键按下了 
#define CT_MAX_TOUCH  5    //电容屏支持的点数,固定为5点

typedef struct
{
	uint8_t (*scan)(uint8_t);				//扫描触摸屏.0,屏幕扫描;1,物理坐标;	 
	uint16_t x[CT_MAX_TOUCH]; 		//当前坐标
	uint16_t y[CT_MAX_TOUCH];		//电容屏有最多5组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标,用
								//x[4],y[4]存储第一次按下时的坐标. 
	uint8_t  sta;					//笔的状态 
								//b7:按下1/松开0; 
	                            //b6:0,没有按键按下;1,有按键按下. 
								//b5:保留
								//b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
#if SYSTEM_SUPPORT_OS
	OS_FLAG_GRP osFlag;		//事件标志组		
#endif
}tp_dev_t;


uint8_t GT1151_Init(void);
uint8_t GT1151_Scan(uint8_t mode); 
tp_dev_t *TPGetDevice(void);
#endif













