#ifndef _I2C_H_
#define _I2C_H_

#include "sys.h"


/*
*********************************************************************
* 此I2C主机控制器支持I2C1和I2C2，他们对应的引脚见下表。
* 其中I2C1通信速率为1M，I2C2通信速率为400K，根据器件所能
* 使用的最大通信速率选择接入哪个主机控制器。
* 两者均使用了DMA传输，其中I2C1用到了DMA1_Stream0和DMA1_Stream1。
* 而I2C2用到了DMA1_Stream2和DMA1_Stream3。
* Code by: Jumping
*********************************************************************
*/

typedef enum
{
	I2C_Transfer_Mode_Poll = 0,		//轮询模式
	I2C_Transfer_Mode_DMA,				//DMA模式	
}I2C_Transfer_ModeTypeDef;

typedef enum
{
	I2C_Transfer_Reg8Bits = 0,
	I2C_Transfer_Reg16Bits,
}I2C_Transfer_RegBits;

typedef enum
{
	I2C_Transfer_TX = 0,			//发送
	I2C_Transfer_RX,					//接收
}I2C_Transfer_Direction;

typedef struct 
{
	I2C_HandleTypeDef hi2c;
}I2C_Handle_TypeDef;

typedef struct
{
  uint8_t devaddr;    //从机地址
  uint16_t regaddr;    //要读写的起始地址
  uint8_t *rxdata;    //指向接收缓冲区
  uint8_t *txdata;    //指向发送缓冲区
  uint16_t rxlen;     //多字节读的长度 - 单字节读时无用
  uint16_t txlen;     //多字节写的长度 - 单字节写时无用
	I2C_Transfer_RegBits rbit;				//寄存器地址位数
	I2C_Transfer_Direction tdir;			//传输方向
	I2C_Transfer_ModeTypeDef tmode;		//传输模式
}I2C_TransferTypeDef;    //I2C传输结构类型


void I2Cx_Master_Init(I2C_Handle_TypeDef *i2c_handle, I2C_TypeDef *i2c);

void I2Cx_Transfer(I2C_Handle_TypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t);



#endif
