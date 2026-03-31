#include "stm32f10x.h"
#include "CJSLDHT11.h"
#include "Delay.h"

/* 引脚定义 - 使用PA7 */
#define DHT11_GPIO_PORT    GPIOA
#define DHT11_GPIO_PIN     GPIO_Pin_7

/* GPIO模式切换：输出/输入上拉 */
static void DHT11_GPIO_Mode(GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if(mode == GPIO_Mode_Out_PP)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    }
    else
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    }

    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/* 起始信号：拉低>=20ms然后释放 */
void DHT11_Start(void)
{
    DHT11_GPIO_Mode(GPIO_Mode_Out_PP);
    GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);  /* 拉低 */
    Delay_ms(20);  /* 实际使用20ms更稳定 */
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);    /* 释放 */
    Delay_us(30);     /* 等待30us */
    DHT11_GPIO_Mode(GPIO_Mode_IPU);  /* 切换为输入 */
}

/* 检测响应信号：83us低电平 + 87us高电平 */
uint8_t DHT11_Check_ACK(void)
{
    uint32_t timeout = 0;

    /* 等待DHT11拉低 (应答低电平83us) */
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_SET)
    {
        timeout++;
        if(timeout > 200) return 1;  /* 超时 */
    }

    timeout = 0;
    /* 等待DHT11拉高 (应答高电平87us) */
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_RESET)
    {
        timeout++;
        if(timeout > 200) return 2;  /* 超时 */
    }

    return 0;  /* 响应正常 */
}

/* 读取1字节数据 - 严格按照官方时序 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat = 0;

    for(i = 0; i < 8; i++)
    {
        /* 等待54us低电平结束 (所有数据位都以54us低电平开头) */
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_RESET);

        /* 官方采样点：低电平结束后，第40us处读电平 */
        /* 位0: 低54us + 高23~27us，40us时仍为高 */
        /* 位1: 低54us + 高68~74us，40us时仍为高 */
        Delay_us(40);  /* 官方推荐的精确采样点 */

        if(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_SET)
        {
            dat |= 1 << (7 - i);  /* 数据位为1 */
        }

        /* 等待高电平结束 (进入下一位的54us低电平) */
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == Bit_SET);
    }

    return dat;
}

/* 读取完整数据 */
uint8_t DHT11_Read(DHT11_Data_TypeDef *data)
{
    uint8_t buf[5];

    DHT11_Start();

    if(DHT11_Check_ACK() != 0)
    {
        return 1;  /* 响应失败 */
    }

    /* 读取5字节数据 */
    buf[0] = DHT11_Read_Byte();  /* 湿度整数 */
    buf[1] = DHT11_Read_Byte();  /* 湿度小数 */
    buf[2] = DHT11_Read_Byte();  /* 温度整数 */
    buf[3] = DHT11_Read_Byte();  /* 温度小数 */
    buf[4] = DHT11_Read_Byte();  /* 校验和 */

    /* 添加调试：如果校验失败，返回实际值用于调试 */
    uint8_t sum = buf[0] + buf[1] + buf[2] + buf[3];

    /* 校验和��证 */
    if(sum == buf[4])
    {
        data->humi_int = buf[0];
        data->humi_dec = buf[1];
        data->temp_int = buf[2];
        data->temp_dec = buf[3];
        data->checksum = buf[4];
        return 0;  /* 读取成功 */
    }
    else
    {
        /* 校验失败时，仍然填充数据（用于调试） */
        data->humi_int = buf[0];
        data->humi_dec = buf[1];
        data->temp_int = buf[2];
        data->temp_dec = buf[3];
        data->checksum = buf[4];
        return 2;  /* 校验失败 */
    }
}

/* DHT11初始化 (上电等待1s) */
void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIOA时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* 配置为推挽输出，默认高电平 */
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);

    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);  /* 默认高电平 */

    /* 上电等待1秒 */
    Delay_s(1);
}

//	while(1)
//	{
//		OLED_Clear();

//		/* 读取DHT11温湿度数据 */
//		uint8_t result = DHT11_Read(&DHT11_Data);

//		if(result == 1)
//		{
//			/* 响应超时，显示错误 */
//			OLED_ShowString(1, 1, "ACK Timeout");
//		}
//		else if(result == 0)
//		{
//			/* 读取成功，显示温度和湿度，带单位符号 */
//			OLED_ShowString(1, 1, "T:");
//			OLED_ShowNum(1, 3, DHT11_Data.temp_int, 2);
//			OLED_ShowChar(1, 5, '.');
//			OLED_ShowNum(1, 6, DHT11_Data.temp_dec, 1);
//			OLED_ShowChar(1, 7, 'C');

//			OLED_ShowString(2, 1, "H:");
//			OLED_ShowNum(2, 3, DHT11_Data.humi_int, 2);
//			OLED_ShowChar(2, 5, '.');
//			OLED_ShowNum(2, 6, DHT11_Data.humi_dec, 1);
//			OLED_ShowChar(2, 7, '%');
//		}
//		else if(result == 2)
//		{
//			/* 校验失败，显示数据但不显示单位符号 */
//			OLED_ShowString(1, 1, "T:");
//			OLED_ShowNum(1, 3, DHT11_Data.temp_int, 2);
//			OLED_ShowChar(1, 5, '.');
//			OLED_ShowNum(1, 6, DHT11_Data.temp_dec, 1);

//			OLED_ShowString(2, 1, "H:");
//			OLED_ShowNum(2, 3, DHT11_Data.humi_int, 2);
//			OLED_ShowChar(2, 5, '.');
//			OLED_ShowNum(2, 6, DHT11_Data.humi_dec, 1);
//		}

//		/* 延时5秒 (官方推荐的稳定间隔>=5s) */
//		Delay_s(5);
//	}
