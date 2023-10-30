#include "i2c.h"

static I2C_HandleTypeDef *hi2c1;
static I2C_HandleTypeDef *hi2c2;
static DMA_HandleTypeDef hdma_i2c1_tx;
static DMA_HandleTypeDef hdma_i2c1_rx;
static DMA_HandleTypeDef hdma_i2c2_tx;
static DMA_HandleTypeDef hdma_i2c2_rx;

static inline void BSP_I2Cx_Write(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t);
static inline void BSP_I2Cx_Write_DMA(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t);
static inline void BSP_I2Cx_Read(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t);
static inline void BSP_I2Cx_Read_DMA(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t);

static void I2C1_Init(I2C_HandleTypeDef *hi2c1);
static void I2C2_Init(I2C_HandleTypeDef *hi2c2);
static void MX_DMA_Init(I2C_TypeDef *i2c);

//初始化I2C为主机模式
void I2Cx_Master_Init(I2C_Handle_TypeDef *i2c_handle, I2C_TypeDef *i2c)
{
  MX_DMA_Init(i2c); //初始化对应的I2C的收发DMA

  switch ((uint32_t)i2c)
  {
    case (uint32_t)I2C1:
      hi2c1 = &i2c_handle->hi2c;
			I2C1_Init(&i2c_handle->hi2c);
      break;
    case (uint32_t)I2C2:
      hi2c2 = &i2c_handle->hi2c;
			I2C2_Init(&i2c_handle->hi2c);
      break;
    default:
      break;
  }
}

//I2C传输顶层函数
void I2Cx_Transfer(I2C_Handle_TypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t)
{
	if (i2c_t->tmode != I2C_Transfer_Mode_DMA) //轮询方式(非DMA方式)
	{
		while(i2c_handle->hi2c.State != HAL_I2C_STATE_READY);		//等待I2C总线空闲
		
		switch (i2c_t->tdir)
		{
		case I2C_Transfer_TX: //发送
			BSP_I2Cx_Write(&i2c_handle->hi2c, i2c_t);
			break;
		case I2C_Transfer_RX: //接收
			BSP_I2Cx_Read(&i2c_handle->hi2c, i2c_t);
			break;
		default:
			break;
		}
	}
	else
	{
		while(i2c_handle->hi2c.State != HAL_I2C_STATE_READY);		//等待I2C总线空闲

		switch (i2c_t->tdir)
		{
		case I2C_Transfer_TX: //发送
			BSP_I2Cx_Write_DMA(&i2c_handle->hi2c, i2c_t);
			break;
		case I2C_Transfer_RX: //接收
			BSP_I2Cx_Read_DMA(&i2c_handle->hi2c, i2c_t);
			break;
		default:
			break;
		}
	}
}

//底层函数 - I2C写
static inline void BSP_I2Cx_Write(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t)
{
	uint16_t reg_bits = I2C_MEMADD_SIZE_8BIT;
	
	if(i2c_t->rbit == I2C_Transfer_Reg16Bits)
		reg_bits = I2C_MEMADD_SIZE_16BIT;
	
  HAL_I2C_Mem_Write(i2c_handle, i2c_t->devaddr, i2c_t->regaddr, reg_bits, i2c_t->txdata, i2c_t->txlen, 1000); //I2C连续写
}

//底层函数 - I2C写(使用DMA)
static inline void BSP_I2Cx_Write_DMA(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t)
{
	uint16_t reg_bits = I2C_MEMADD_SIZE_8BIT;
	
	if(i2c_t->rbit == I2C_Transfer_Reg16Bits)
		reg_bits = I2C_MEMADD_SIZE_16BIT;
	
  HAL_I2C_Mem_Write_DMA(i2c_handle, i2c_t->devaddr, i2c_t->regaddr, reg_bits, i2c_t->txdata, i2c_t->txlen);
}

//底层函数 - I2C读
static inline void BSP_I2Cx_Read(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t)
{
	uint16_t reg_bits = I2C_MEMADD_SIZE_8BIT;
	
	if(i2c_t->rbit == I2C_Transfer_Reg16Bits)
		reg_bits = I2C_MEMADD_SIZE_16BIT;
	
  HAL_I2C_Mem_Read(i2c_handle, i2c_t->devaddr, i2c_t->regaddr, reg_bits, i2c_t->rxdata, i2c_t->rxlen, 1000);
}

//底层函数 - I2C读(DMA)
static inline void BSP_I2Cx_Read_DMA(I2C_HandleTypeDef *i2c_handle, I2C_TransferTypeDef *i2c_t)
{
	uint16_t reg_bits = I2C_MEMADD_SIZE_8BIT;
	
	if(i2c_t->rbit == I2C_Transfer_Reg16Bits)
		reg_bits = I2C_MEMADD_SIZE_16BIT;
	
  HAL_I2C_Mem_Read_DMA(i2c_handle, i2c_t->devaddr, i2c_t->regaddr, reg_bits, i2c_t->rxdata, i2c_t->rxlen);
}

//I2C1主机控制器初始化
static void I2C1_Init(I2C_HandleTypeDef *hi2c1)
{
  hi2c1->Instance = I2C1;
  hi2c1->Init.Timing = 0x40B70E22;
  hi2c1->Init.OwnAddress1 = 0;
  hi2c1->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1->Init.OwnAddress2 = 0;
  hi2c1->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	
  HAL_I2C_Init(hi2c1);

  HAL_I2CEx_ConfigAnalogFilter(hi2c1, I2C_ANALOGFILTER_ENABLE);
	
	HAL_I2CEx_DigitalFilter_Config(hi2c1, 10);
}
//I2C2主机控制器初始化
static void I2C2_Init(I2C_HandleTypeDef *hi2c2)
{
  hi2c2->Instance = I2C2;
  hi2c2->Init.Timing = 0x00D04BFF;
  hi2c2->Init.OwnAddress1 = 0;
  hi2c2->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2->Init.OwnAddress2 = 0;
  hi2c2->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(hi2c2);

  HAL_I2CEx_ConfigAnalogFilter(hi2c2, I2C_ANALOGFILTER_ENABLE);
}
											                           
//I2C初始化回调函数
void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (i2cHandle->Instance == I2C1)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 DMA Init */
    /* I2C1_TX Init */
    hdma_i2c1_tx.Instance = DMA1_Stream0;
    hdma_i2c1_tx.Init.Request = DMA_REQUEST_I2C1_TX;
    hdma_i2c1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2c1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c1_tx);

    __HAL_LINKDMA(i2cHandle, hdmatx, hdma_i2c1_tx);

    hdma_i2c1_rx.Instance = DMA1_Stream1;
    hdma_i2c1_rx.Init.Request = DMA_REQUEST_I2C1_RX;
    hdma_i2c1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2c1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c1_rx);

    __HAL_LINKDMA(i2cHandle, hdmarx, hdma_i2c1_rx);

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  }
  else if (i2cHandle->Instance == I2C2)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C2 GPIO Configuration                                                                                                   
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();

    /* I2C2 DMA Init */
    /* I2C2_TX Init */
    hdma_i2c2_tx.Instance = DMA1_Stream2;
    hdma_i2c2_tx.Init.Request = DMA_REQUEST_I2C2_TX;
    hdma_i2c2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2c2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c2_tx.Init.Mode = DMA_NORMAL;
    hdma_i2c2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2c2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c2_tx);

    __HAL_LINKDMA(i2cHandle, hdmatx, hdma_i2c2_tx);

    /* I2C2_RX Init */
    hdma_i2c2_rx.Instance = DMA1_Stream3;
    hdma_i2c2_rx.Init.Request = DMA_REQUEST_I2C2_RX;
    hdma_i2c2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c2_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c2_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_i2c2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c2_rx);

    __HAL_LINKDMA(i2cHandle, hdmarx, hdma_i2c2_rx);

    /* I2C2 interrupt Init */
    HAL_NVIC_SetPriority(I2C2_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_SetPriority(I2C2_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
  }
}

//DMA初始化
static void MX_DMA_Init(I2C_TypeDef *i2c)
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  switch ((uint32_t)i2c)
  {
    case (uint32_t)I2C1:
      HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

      HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
      break;
    case (uint32_t)I2C2:
      HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);

      HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
      break;
    default:
      break;
  }
}

//DMA1数据流0中断服务函数
void DMA1_Stream0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}

//DMA1数据流1中断服务函数
void DMA1_Stream1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}

//DMA1数据流2中断服务函数
void DMA1_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_i2c2_tx);
}

//DMA1数据流3中断服务函数
void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_i2c2_rx);
}

//I2C1事件中断服务函数
void I2C1_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(hi2c1);
}

//I2C1错误中断服务函数
void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(hi2c1);
}

//I2C2事件中断服务函数
void I2C2_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(hi2c2);
}

//I2C2错误中断服务函数
void I2C2_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(hi2c2);
}

//I2C2错误中断回调函数
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_DeInit(hi2c);
	
	switch ((uint32_t)(hi2c->Instance))
  {
    case (uint32_t)I2C1:
			
			I2C1_Init(hi2c);   		
      break;
    case (uint32_t)I2C2:
			I2C2_Init(hi2c);
      break;
    default:
      break;
  }
	
}


