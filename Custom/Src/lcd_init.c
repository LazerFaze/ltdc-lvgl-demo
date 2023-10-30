#include "lcd_init.h"
#include "tim.h"
#include "lvgl.h"

static LTDC_HandleTypeDef hltdc;
static DMA2D_HandleTypeDef hdma2d;

static void LCD_RGBConfig(void);
static void LTDC_Init(void);
static void DMA2D_Init(void);

static uint32_t frambuffer; // 前缓冲区
static volatile uint8_t g_gpu_state = 0;

// 初始化LCD
void LCD_Init(uint32_t fb)
{
  frambuffer = fb; // 设置前缓冲区
  DMA2D_Init();
  LTDC_Init();
  LCD_RGBConfig();
  LCD_Brightness_Timer_Init(); // 背光控制初始化

  LCD_BrightnessSetPulse(70); // 设置背光
}

// LCD单色填充
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
  uint32_t psx = sx, psy = sy, pex = ex, pey = ey; // 以LCD面板为基准的坐标系,不随横竖屏变化而变化
  uint32_t timeout = 0;
  uint16_t offline;
  uint32_t addr;

  offline = LCD_WIDTH - (pex - psx + 1);
  addr = frambuffer + 16 * (LCD_WIDTH * psy + psx);
  RCC->AHB1ENR |= 1 << 23;                                // 使能DM2D时钟
  DMA2D->CR &= ~(1 << 0);                                 // 先停止DMA2D
  DMA2D->CR = 3 << 16;                                    // 寄存器到存储器模式
  DMA2D->OPFCCR = 0x02;                                   // 设置颜色格式RGB565
  DMA2D->OOR = offline;                                   // 设置行偏移
  DMA2D->OMAR = addr;                                     // 输出存储器地址
  DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16); // 设定行数寄存器
  DMA2D->OCOLR = color;                                   // 设定输出颜色寄存器
  DMA2D->CR |= 1 << 0;                                    // 启动DMA2D
  while ((DMA2D->ISR & (1 << 1)) == 0)                    // 等待传输完成
  {
    timeout++;
    if (timeout > 0X1FFFFF)
      break; // 超时退出
  }
  DMA2D->IFCR |= 1 << 1; // 清除传输完成标志
}

// 查询方式填充颜色块
void LCD_Color_Fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t *color)
{
  DMA2D->CR &= ~(DMA2D_CR_START);            //	停止DMA2D
  DMA2D->CR = DMA2D_M2M;                     //	存储器到存储器
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565; //	设置颜色格式
  DMA2D->FGOR = 0;                           //
  DMA2D->OOR = LCD_WIDTH - width;            //	设置行偏移
  DMA2D->FGMAR = (uint32_t)color;
  DMA2D->OMAR = frambuffer + 2 * (LCD_WIDTH * y + x); // 地址;
  DMA2D->NLR = (width << 16) | (height);              //	设定长度和宽度
  DMA2D->CR |= DMA2D_CR_START;                        //	启动DMA2D

  while (DMA2D->CR & DMA2D_CR_START)
    ; //	等待传输完成
}

// 中断方式填充颜色块
void LCD_Color_Fill_IT(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t *color)
{
  HAL_NVIC_SetPriority(DMA2D_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(DMA2D_IRQn);
  DMA2D->CR &= ~(DMA2D_CR_START);            //	停止DMA2D
  DMA2D->CR = DMA2D_M2M;                     //	存储器到存储器
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565; //	设置颜色格式
  DMA2D->FGOR = 0;                           //
  DMA2D->OOR = LCD_WIDTH - width;            //	设置行偏移
  DMA2D->FGMAR = (uint32_t)color;
  DMA2D->OMAR = frambuffer + 2 * (LCD_WIDTH * y + x);   // 地址;
  DMA2D->NLR = (width << 16) | (height);                //	设定长度和宽度
  DMA2D->CR |= DMA2D_IT_TC | DMA2D_IT_TE | DMA2D_IT_CE; // 开启中断
  DMA2D->CR |= DMA2D_CR_START;                          //	启动DMA2D
  g_gpu_state = 1;
}

// 设置LCD帧缓冲区并开启重装载中断
void LCD_SetFb_IT(uint32_t fb)
{
  HAL_LTDC_SetAddress_NoReload(&hltdc, fb, LTDC_LAYER_1);
  
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);   // 开启重装载中断
}

__weak void LCD_SetFb_Cplt(void)
{
  
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
  LCD_SetFb_Cplt();   //设置完成
}

static void LCD_SPI_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  LCD_CS_PORT_CLK_ENABLE();
  LCD_SDA_PORT_CLK_ENABLE();
  LCD_SCL_PORT_CLK_ENABLE();

  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pin = LCD_CS_PIN;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.Pin = LCD_SDA_PIN;
  HAL_GPIO_Init(LCD_SDA_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.Pin = LCD_SCL_PIN;
  HAL_GPIO_Init(LCD_SCL_PORT, &GPIO_InitStructure);
}

static void Write_3Spi_cmd(uint16_t index)
{
  uint8_t i, cmd;
  uint16_t s;
  cmd = 0x70;
  s = index;
  LCD_SDA(0);    // SDI_L;
  LCD_SCL(0);    // SCL_L;
  delay_us(100); // Delayns(100);
  LCD_CS(0);     // CS_L;
  delay_us(500); // Delayns(500);
  for (i = 0; i < 8; i++)
  {
    LCD_SCL(0); // SCL_L;

    if ((cmd & 0x80) != 0)
      LCD_SDA(1); // SDI_H;

    else
      LCD_SDA(0); // SDI_L;

    delay_us(10);
    LCD_SCL(1); // SCL_H;
    cmd <<= 1;
    delay_us(10);
  }

  for (i = 0; i < 16; i++)
  {
    LCD_SCL(0); // SCL_L;
    if ((s & 0x8000) != 0)
      LCD_SDA(1); // SDI_H;
    else
      LCD_SDA(0); // SDI_L;

    delay_us(10);
    LCD_SCL(1); // SCL_H;
    s <<= 1;
    delay_us(10);
  }
  delay_us(500);
  LCD_CS(1);  // CS_H;
  LCD_SCL(0); // SCL_L;
  LCD_SDA(0); // SDI_L;
  delay_us(100);
}

static void Write_3Spi_data(uint16_t data)

{
  uint8_t i, cmd;
  uint16_t t;
  cmd = 0x72;
  t = data;
  LCD_SDA(0); // SDI_L;
  LCD_SCL(0); // SCL_L;
  delay_us(100);
  LCD_CS(0); // CS_L;
  delay_us(500);
  for (i = 0; i < 8; i++)
  {
    LCD_SCL(0); // SCL_L;

    if ((cmd & 0x80) != 0)
      LCD_SDA(1); // SDI_H;

    else
      LCD_SDA(0); // SDI_L;

    delay_us(10);

    LCD_SCL(1); // SCL_H;
    cmd <<= 1;
    delay_us(10);
  }

  for (i = 0; i < 16; i++)
  {
    LCD_SCL(0); // SCL_L;

    if ((t & 0x8000) != 0)
      LCD_SDA(1); // SDI_H;
    else
      LCD_SDA(0); // SDI_L;

    delay_us(10);
    LCD_SCL(1); // SCL_H;
    t <<= 1;
    delay_us(10);
  }

  delay_us(500);
  LCD_CS(1);  // CS_H;
  LCD_SCL(0); // SCL_L;
  LCD_SDA(0); // SDI_L;

  delay_us(100);
}

// 配置LCD为RGB接口
static void LCD_RGBConfig(void)
{
  LCD_SPI_Init();

#if 0
	
		Write_3Spi_cmd(0x0001); Write_3Spi_data(0x6300);//6300   6B00
    Write_3Spi_cmd(0x0002); Write_3Spi_data(0x0200);
    Write_3Spi_cmd(0x0003); Write_3Spi_data(0x6564);
    Write_3Spi_cmd(0x0004); Write_3Spi_data(0x04c7); //
    Write_3Spi_cmd(0x0005); Write_3Spi_data(0xb4d4); //b4d4
    Write_3Spi_cmd(0x0008); Write_3Spi_data(0x06ff); 
    Write_3Spi_cmd(0x000A); Write_3Spi_data(0x4008);//
    delay_ms(150);
    Write_3Spi_cmd(0x000B); Write_3Spi_data(0xD400);
    Write_3Spi_cmd(0x000D); Write_3Spi_data(0x1220); //
    delay_ms(150);
    Write_3Spi_cmd(0x000E); Write_3Spi_data(0x1000);  //
    delay_ms(150);
    Write_3Spi_cmd(0x000F); Write_3Spi_data(0x0000);
    Write_3Spi_cmd(0x0016); Write_3Spi_data(0x9F80);
    Write_3Spi_cmd(0x0017); Write_3Spi_data(0x2212);//2212
    delay_ms(150);
    Write_3Spi_cmd(0x001E); Write_3Spi_data(0x00F0); //
    delay_ms(150);
    Write_3Spi_cmd(0x0030); Write_3Spi_data(0x0000);
    Write_3Spi_cmd(0x0031); Write_3Spi_data(0x0707);
    Write_3Spi_cmd(0x0032); Write_3Spi_data(0x0206);
    Write_3Spi_cmd(0x0033); Write_3Spi_data(0x0001);
    Write_3Spi_cmd(0x0034); Write_3Spi_data(0x0105);
    Write_3Spi_cmd(0x0035); Write_3Spi_data(0x0000);
    Write_3Spi_cmd(0x0036); Write_3Spi_data(0x0707);
    Write_3Spi_cmd(0x0037); Write_3Spi_data(0x0100);
    Write_3Spi_cmd(0x003A); Write_3Spi_data(0x0502);  //
    Write_3Spi_cmd(0x003B); Write_3Spi_data(0x0502);  //
    delay_ms(250);

#endif

#if 0


	Write_3Spi_cmd(0x0001);Write_3Spi_data(0x6300);//6300   
	Write_3Spi_cmd(0x0002);Write_3Spi_data(0x0200);
	Write_3Spi_cmd(0x0003);Write_3Spi_data(0x6564);//6564
	Write_3Spi_cmd(0x0004);Write_3Spi_data(0x04c7);//04C7
	Write_3Spi_cmd(0x0005);Write_3Spi_data(0xB4D4);//B4D4  B0D4
	Write_3Spi_cmd(0x0008);Write_3Spi_data(0x06ff);
	Write_3Spi_cmd(0x000A);Write_3Spi_data(0x4008);
	Write_3Spi_cmd(0x000B);Write_3Spi_data(0xd400);

	Write_3Spi_cmd(0x000D);Write_3Spi_data(0x3229);
	Write_3Spi_cmd(0x000E);Write_3Spi_data(0x1100);
	Write_3Spi_cmd(0x000F);Write_3Spi_data(0x0000);
	Write_3Spi_cmd(0x0016);Write_3Spi_data(0x9f80);
	Write_3Spi_cmd(0x0017);Write_3Spi_data(0x0812);//HBP  VBP   2212
	Write_3Spi_cmd(0x001E);Write_3Spi_data(0x00F3);

	Write_3Spi_cmd(0x0030);Write_3Spi_data(0x0000);
	Write_3Spi_cmd(0x0031);Write_3Spi_data(0x0707);
	Write_3Spi_cmd(0x0032);Write_3Spi_data(0x0206);
	Write_3Spi_cmd(0x0033);Write_3Spi_data(0x0001);
	Write_3Spi_cmd(0x0034);Write_3Spi_data(0x0105);
	Write_3Spi_cmd(0x0035);Write_3Spi_data(0x0000);
	Write_3Spi_cmd(0x0036);Write_3Spi_data(0x0707);
	Write_3Spi_cmd(0x0037);Write_3Spi_data(0x0100);
	Write_3Spi_cmd(0x003A);Write_3Spi_data(0x0502);
	Write_3Spi_cmd(0x003B);Write_3Spi_data(0x0502);

	delay_ms(200);

#endif

#if 1

  Write_3Spi_cmd(0x0001);
  Write_3Spi_data(0x6300);
  Write_3Spi_cmd(0x0002);
  Write_3Spi_data(0x0200);
  Write_3Spi_cmd(0x0003);
  Write_3Spi_data(0x6564);
  Write_3Spi_cmd(0x0004);
  Write_3Spi_data(0x04c7); // 8bit serial RGB
  Write_3Spi_cmd(0x0005);
  Write_3Spi_data(0xA400); // A800
  Write_3Spi_cmd(0x0008);
  Write_3Spi_data(0x06ff);
  Write_3Spi_cmd(0x000A);
  Write_3Spi_data(0x4008);
  Write_3Spi_cmd(0x000B);
  Write_3Spi_data(0xd400);

  Write_3Spi_cmd(0x000D);
  Write_3Spi_data(0x3229);
  Write_3Spi_cmd(0x000E);
  Write_3Spi_data(0x1100);
  Write_3Spi_cmd(0x000F);
  Write_3Spi_data(0x0000);
  Write_3Spi_cmd(0x0016);
  Write_3Spi_data(0x9f80);
  Write_3Spi_cmd(0x0017);
  Write_3Spi_data(0x2212);
  Write_3Spi_cmd(0x001E);
  Write_3Spi_data(0x00F5); // VCOMH   00F3

  Write_3Spi_cmd(0x0030);
  Write_3Spi_data(0x0000);
  Write_3Spi_cmd(0x0031);
  Write_3Spi_data(0x0707);
  Write_3Spi_cmd(0x0032);
  Write_3Spi_data(0x0206);
  Write_3Spi_cmd(0x0033);
  Write_3Spi_data(0x0001);
  Write_3Spi_cmd(0x0034);
  Write_3Spi_data(0x0105);
  Write_3Spi_cmd(0x0035);
  Write_3Spi_data(0x0000);
  Write_3Spi_cmd(0x0036);
  Write_3Spi_data(0x0707);
  Write_3Spi_cmd(0x0037);
  Write_3Spi_data(0x0100);
  Write_3Spi_cmd(0x003A);
  Write_3Spi_data(0x0502);
  Write_3Spi_cmd(0x003B);
  Write_3Spi_data(0x0502);

#endif
}

/* LTDC init function */
static void LTDC_Init(void)
{
  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AH;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;
  hltdc.Init.HorizontalSync = 0;
  hltdc.Init.VerticalSync = 0;
  hltdc.Init.AccumulatedHBP = 86;
  hltdc.Init.AccumulatedVBP = 17;
  hltdc.Init.AccumulatedActiveW = 406;
  hltdc.Init.AccumulatedActiveH = 257;
  hltdc.Init.TotalWidth = 426;
  hltdc.Init.TotalHeigh = 261;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    while (1)
      ;
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 320;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 240;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = (uint32_t)frambuffer; // 帧缓存区首地址
  pLayerCfg.ImageWidth = 320;
  pLayerCfg.ImageHeight = 240;
  pLayerCfg.Backcolor.Blue = 255;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    while (1)
      ;
  }

  HAL_NVIC_SetPriority(LTDC_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

// DMA2D传输完成回调
static void mDMA2Dcallvack(DMA2D_HandleTypeDef *hdma2d)
{
  if (g_gpu_state == 1)
  {
    g_gpu_state = 0;
  }
}

/* DMA2D init function */
static void DMA2D_Init(void)
{
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.Init.BytesSwap = DMA2D_BYTES_REGULAR;
  hdma2d.Init.LineOffsetMode = DMA2D_LOM_PIXELS;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
  hdma2d.XferCpltCallback = mDMA2Dcallvack;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    while (1)
      ;
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    while (1)
      ;
  }
}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *ltdcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (ltdcHandle->Instance == LTDC)
  {
    /* USER CODE BEGIN LTDC_MspInit 0 */

    /* USER CODE END LTDC_MspInit 0 */
    /* LTDC clock enable */
    __HAL_RCC_LTDC_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**LTDC GPIO Configuration
    PC0     ------> LTDC_G2
    PA4     ------> LTDC_VSYNC
    PA5     ------> LTDC_R4
    PC4     ------> LTDC_R7
    PC5     ------> LTDC_DE
    PB0     ------> LTDC_R3
    PE11     ------> LTDC_G3
    PE12     ------> LTDC_B4
    PE14     ------> LTDC_CLK
    PB10     ------> LTDC_G4
    PB11     ------> LTDC_G5
    PD10     ------> LTDC_B3
    PC6     ------> LTDC_HSYNC
    PC7     ------> LTDC_G6
    PA8     ------> LTDC_R6
    PA9     ------> LTDC_R5
    PA15     ------> LTDC_B6
    PD3     ------> LTDC_G7
    PB5     ------> LTDC_B5
    PB9     ------> LTDC_B7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USER CODE BEGIN LTDC_MspInit 1 */

    /* USER CODE END LTDC_MspInit 1 */
  }
}

void HAL_DMA2D_MspInit(DMA2D_HandleTypeDef *dma2dHandle)
{

  if (dma2dHandle->Instance == DMA2D)
  {
    /* DMA2D clock enable */
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /* DMA2D interrupt Init */
    HAL_NVIC_SetPriority(DMA2D_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);
  }
}

void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc);
}

void DMA2D_IRQHandler(void)
{
  HAL_DMA2D_IRQHandler(&hdma2d);
}
