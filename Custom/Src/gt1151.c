#include "gt1151.h"
#include "i2c.h"
#include "string.h"
#include "sys.h"
#include "lcd_init.h"
#include "stdbool.h"

static void CT_BSP_Init(void);
static void CT_INT_FLOATING(void);
static uint8_t GT1151_Send_Cfg(uint8_t mode);
static uint8_t GT1151_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len);
static void GT1151_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len);

static I2C_Handle_TypeDef i2c1_handle;
static tp_dev_t tp_dev; // 定义一个触摸结构
static volatile bool int_flag = false;

// GT1151配置参数表
// 第一个字节为版本号(0X60),必须保证新的版本号大于等于GT1151内部
// flash原有版本号,才会更新配置.
// x坐标输出最大值0x0140=320
// y坐标输出最大值0x00F0=240
const uint8_t GT1151_CFG_TBL[] =
	{
		0x42,
		0x40,
		0x01,
		0xF0,
		0x00,
		0x05,
		0x0C,
		0x00,
		0x01,
		0x00,
		0x00,
		0x05,
		0x50,
		0x3C,
		0x53,
		0x11,
		0x00,
		0x00,
		0x00,
		0x00,
		0x14,
		0x14,
		0x14,
		0x22,
		0x0A,
		0x04,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x53,
		0x00,
		0x14,
		0x00,
		0x00,
		0x84,
		0x00,
		0x00,
		0x3C,
		0x19,
		0x19,
		0x64,
		0x1E,
		0x28,
		0x86,
		0x06,
		0x09,
		0x3E,
		0x3C,
		0x33,
		0x0F,
		0x20,
		0x33,
		0x60,
		0x12,
		0x02,
		0x24,
		0x00,
		0x00,
		0x30,
		0x77,
		0x80,
		0x14,
		0x02,
		0x00,
		0x00,
		0x54,
		0x8A,
		0x68,
		0x86,
		0x6D,
		0x83,
		0x72,
		0x7F,
		0x76,
		0x7D,
		0x7B,
		0x7B,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0xF0,
		0x50,
		0x3C,
		0xFF,
		0xFF,
		0x07,
		0x00,
		0x00,
		0x00,
		0x02,
		0x14,
		0x14,
		0x03,
		0x04,
		0x00,
		0x21,
		0x64,
		0x0A,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x32,
		0x20,
		0x50,
		0x3C,
		0x3C,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x0D,
		0x06,
		0x0C,
		0x05,
		0x0B,
		0x04,
		0x0A,
		0x03,
		0x09,
		0x02,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0x00,
		0x01,
		0x02,
		0x03,
		0x04,
		0x05,
		0x06,
		0x07,
		0x08,
		0x09,
		0x0A,
		0x0C,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x3C,
		0x00,
		0x05,
		0x1E,
		0x00,
		0x02,
		0x2A,
		0x1E,
		0x19,
		0x14,
		0x02,
		0x00,
		0x03,
		0x0A,
		0x05,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x01,
		0xFF,
		0xFF,
		0x86,
		0x22,
		0x03,
		0x00,
		0x00,
		0x33,
		0x00,
		0x0F,
		0x00,
		0x00,
		0x00,
		0x50,
		0x3C,
		0x50,
		0x00,
		0x00,
		0x00,
		0x5A,
		0x0F,
		0x01,
};

// 发送GT1151配置参数
// mode:0,参数不保存到flash
//      1,参数保存到flash
uint8_t GT1151_Send_Cfg(uint8_t mode)
{
	uint16_t checksum = 0;
	uint8_t buf[3];
	uint8_t i = 0;
	for (i = 0; i < (sizeof(GT1151_CFG_TBL) - 3); i += 2)
		checksum += (GT1151_CFG_TBL[i] << 8) + GT1151_CFG_TBL[i + 1]; // 计算校验和
	checksum = 0 - checksum;
	// printf("Bytesum:%d,\r\n",sizeof(GT1151_CFG_TBL));
	printf("chksum:0x%x,\r\n", checksum);
	buf[0] = checksum >> 8;
	buf[1] = checksum;
	buf[2] = mode; // 是否写入到GT1151 FLASH?  即是否掉电保存
	printf("chksum_H:0x%x,\r\n", buf[0]);
	printf("chksum_L:0x%x,\r\n", buf[1]);
	// printf("\r\ncrc=%x",CRC16((uint8_t*)GT1151_CFG_TBL,sizeof(GT1151_CFG_TBL)));
	GT1151_WR_Reg(GT_CFGS_REG, (uint8_t *)GT1151_CFG_TBL, sizeof(GT1151_CFG_TBL)); // 发送寄存器配置
	// GT1151_WR_Reg(GT_CHECK_REG,buf,3);//写入校验和,和配置更新标记
	return 0;
}

// 初始化GT1151触摸屏
// 返回值:0,初始化成功;1,初始化失败
uint8_t GT1151_Init(void)
{
	uint8_t temp[5];
	uint8_t i = 0;
	uint8_t Cfg_Info[239] = {0};

	CT_BSP_Init(); // 初始化I2C，GPIO等

	GT_RST(0); // 复位
	delay_ms(10);
	GT_RST(1); // 释放复位
	delay_ms(10);
	CT_INT_FLOATING();
	delay_ms(100);

	GT1151_RD_Reg(GT_PID_REG, temp, 4); // 读取产品ID
	temp[4] = '\0';
	printf("CTP ID:0x%s\r\n", temp); // 打印LCD ID

	GT1151_RD_Reg(GT_CFGS_REG, Cfg_Info, sizeof(Cfg_Info));
	printf("Config Info:\r\n");
	for (i = 0; i < sizeof(Cfg_Info); i++)
	{
		if (Cfg_Info[i] < 0x10)
			printf("0x0%x, ", Cfg_Info[i]);
		else
			printf("0x%x, ", Cfg_Info[i]);
		if (((i + 1) % 0x0a) == 0)
			printf("\r\n");
	}
	printf("\r\n");

	if (strcmp((char *)temp, "1158") == 0) // ID==1158
	{
		temp[0] = 0X02;
		GT1151_WR_Reg(GT_CTRL_REG, temp, 1); // 软复位GT1151
		GT1151_RD_Reg(GT_CFGS_REG, temp, 1); // 读取GT_CFGS_REG寄存器
		//		if(temp[0]<0X60)//默认版本比较低,需要更新flash配置
		//		{
		printf("Default Ver:%x\r\n", temp[0]);
		//			GT1151_Send_Cfg(1);
		//			if(lcddev.id==0X7084)GT1151_Send_Cfg(1);//仅4.3寸MCU屏,更新并保存配置
		//		}
		delay_ms(10);
		temp[0] = 0X00;
		GT1151_WR_Reg(GT_CTRL_REG, temp, 1); // 结束复位
		tp_dev.scan = GT1151_Scan;

		return 0;
	}
	return 1;
}
static const uint16_t GT1151_TPX_TBL[5] = {GT_TP1_REG, GT_TP2_REG, GT_TP3_REG, GT_TP4_REG, GT_TP5_REG};
// 扫描触摸屏(采用查询方式)
// mode:0,正常扫描.
// 返回值:当前触屏状态.
// 0,触屏无触摸;1,触屏有触摸
uint8_t GT1151_Scan(uint8_t mode)
{
	uint8_t buf[4];
	uint8_t i = 0;
	uint8_t res = 0;
	uint8_t temp;
	uint8_t tempsta;

	GT1151_RD_Reg(GT_GSTID_REG, &mode, 1); // 读取触摸点的状态
	if (mode & 0X80 && ((mode & 0XF) < 6))
	{
		temp = 0;
		GT1151_WR_Reg(GT_GSTID_REG, &temp, 1); // 清标志
	}
	if ((mode & 0XF) && ((mode & 0XF) < 6))
	{
		temp = 0XFF << (mode & 0XF); // 将点的个数转换为1的位数,匹配tp_dev.sta定义
		tempsta = tp_dev.sta;		 // 保存当前的tp_dev.sta值
		tp_dev.sta = (~temp) | TP_PRES_DOWN | TP_CATH_PRES;
		tp_dev.x[4] = tp_dev.x[0]; // 保存触点0的数据
		tp_dev.y[4] = tp_dev.y[0];
		for (i = 0; i < 5; i++)
		{
			if (tp_dev.sta & (1 << i)) // 触摸有效?
			{
				GT1151_RD_Reg(GT1151_TPX_TBL[i], buf, 4); // 读取XY坐标值
				tp_dev.x[i] = (((uint16_t)buf[1] << 8) + buf[0]);
				tp_dev.y[i] = (((uint16_t)buf[3] << 8) + buf[2]);
				//					printf("x[%d]:%d,y[%d]:%d\r\n",i,tp_dev.x[i],i,tp_dev.y[i]);
			}
		}
		res = 1;
		if (tp_dev.x[0] > LCD_WIDTH || tp_dev.y[0] > LCD_HEIGHT) // 非法数据(坐标超出了)
		{
			if ((mode & 0XF) > 1) // 有其他点有数据,则复第二个触点的数据到第一个触点.
			{
				tp_dev.x[0] = tp_dev.x[1];
				tp_dev.y[0] = tp_dev.y[1];
			}
			else // 非法数据,则忽略此次数据(还原原来的)
			{
				tp_dev.x[0] = tp_dev.x[4];
				tp_dev.y[0] = tp_dev.y[4];
				mode = 0X80;
				tp_dev.sta = tempsta; // 恢复tp_dev.sta
			}
		}
	}
	if ((mode & 0X8F) == 0X80) // 无触摸点按下
	{
		if (tp_dev.sta & TP_PRES_DOWN) // 之前是被按下的
		{
			tp_dev.sta &= ~(1 << 7); // 标记按键松开
		}
		else // 之前就没有被按下
		{
			tp_dev.x[0] = 0xffff;
			tp_dev.y[0] = 0xffff;
			tp_dev.sta &= 0XE0; // 清除点有效标记
		}
	}

	return res;
}

tp_dev_t *TPGetDevice(void)
{
	return &tp_dev;
}

// 向GT1151写入一次数据
// 返回值:0,成功;1,失败.
static uint8_t GT1151_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
	I2C_TransferTypeDef i2c_t;

	i2c_t.devaddr = GT_ADDR;
	i2c_t.regaddr = reg;
	i2c_t.txdata = buf;
	i2c_t.txlen = len;
	i2c_t.rbit = I2C_Transfer_Reg16Bits;
	i2c_t.tdir = I2C_Transfer_TX;
	i2c_t.tmode = I2C_Transfer_Mode_Poll;

	I2Cx_Transfer(&i2c1_handle, &i2c_t);

	return 0;
}
// 从GT1151读出一次数据
static void GT1151_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
	I2C_TransferTypeDef i2c_t;

	i2c_t.devaddr = GT_ADDR;
	i2c_t.regaddr = reg;
	i2c_t.rxdata = buf;
	i2c_t.rxlen = len;
	i2c_t.rbit = I2C_Transfer_Reg16Bits;
	i2c_t.tdir = I2C_Transfer_RX;
	i2c_t.tmode = I2C_Transfer_Mode_Poll;

	I2Cx_Transfer(&i2c1_handle, &i2c_t);
}

static void CT_BSP_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GT_INT_PORT_CLK_ENABLE();
	GT_RST_PORT_CLK_ENABLE();

	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pin = GT_RST_PIN;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GT_RST_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Pin = GT_INT_PIN;
	HAL_GPIO_Init(GT_INT_PORT, &GPIO_InitStructure);

	I2Cx_Master_Init(&i2c1_handle, I2C1);

	GT_RST(0);
}

static void CT_INT_FLOATING(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pin = GT_INT_PIN;
	HAL_GPIO_Init(GT_INT_PORT, &GPIO_InitStructure);

	HAL_NVIC_SetPriority(GT_INT_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(GT_INT_IRQn);
}

// 触摸中断IO中断服务函数
void GT_INT_IRQHandler(void)
{
	if (GT_INTR() != true)
	{
		tp_dev.sta |= TP_PRES_DOWN; // 触摸坐标有更新
	}

	__HAL_GPIO_EXTI_CLEAR_IT(GT_INT_PIN); // 清除中断标志位
}
