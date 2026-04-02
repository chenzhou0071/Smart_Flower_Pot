#include "stm32f10x.h"                  // Device header
#include "WaterPump.h"
#include "Delay.h"

/**
 * @brief  抽水泵初始化函数
 * @param  无
 * @retval 无
 */
void WaterPump_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置GPIOA.6为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);
}

/**
 * @brief  抽水泵控制函数
 * @param  state: 控制状态，1为开启，0为关闭
 * @retval 无
 */
void WaterPump_Control(uint8_t state)
{
    if (state)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_6);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_6);
    }
}

/**
 * @brief  抽水泵控制函数
 * @param  state: 开水泵0.5s；
 * @retval 无
 */
void WaterPump_Open()
{
    WaterPump_Control(1);
	Delay_ms(500);
	WaterPump_Control(0);
}