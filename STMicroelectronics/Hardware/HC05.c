#include "HC05.h"
#include "Delay.h"
#include <string.h>
#include <stdio.h>

/* HC-05全局变量 */
HC05_TypeDef HC05;

/* 内部函数声明 */
static void HC05_GPIO_Init(void);
static void HC05_USART_Init(uint32_t baudrate);
void HC05_USART_IRQHandler(void);

/**
 * @brief HC-05模块初始化（默认38400波特率）
 */
void HC05_Init(void)
{
    /* 初始化结构体 */
    memset(&HC05, 0, sizeof(HC05_TypeDef));
    HC05.mode = HC05_MODE_DISCONNECTED;

    /* 初始化GPIO和USART */
    HC05_GPIO_Init();

    /* 设置为已初��化 */
    HC05.initialized = 1;
    HC05.connected = 0;
}

/**
 * @brief HC-05进入AT模式并初始化配置
 * 注意：必须手动将HC-05进入AT模式（长按按键上电）
 * AT模式波特率固定为38400，不能更改
 */
void HC05_AT_Init(void)
{
    char response[100];

    /* 确认已手动进入AT模式 */
    HC05_SetATMode(1);
    Delay_ms(500);

    /* 测试AT指令（波特率必须是38400）*/
    HC05_SendATCommand("AT\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 设置设备名称 */
    HC05_SendATCommand("AT+NAME=IrrigationSystem\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 设置通信波特率为115200（AT指令波特率仍为38400）*/
    HC05_SendATCommand("AT+BAUD=8\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 设置角色为从机 */
    HC05_SendATCommand("AT+ROLE=0\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 设置配对密码（4位数字）*/
    HC05_SendATCommand("AT+PSWD=1234\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 设置连接模式为固定连接 */
    HC05_SendATCommand("AT+CMODE=0\r\n", response, sizeof(response));
    Delay_ms(100);

    /* 退出AT模式 - 重新上电HC-05模块 */
    HC05_SetATMode(0);
    Delay_ms(100);

    /* 重新初始化USART为新的通信波特率115200 */
    HC05_USART_Init(115200);

    HC05.initialized = 1;
}

/**
 * @brief 发送AT指令并接收响应
 * 注意：AT指令波特率固定为38400
 */
void HC05_SendATCommand(char *cmd, char *response, uint16_t response_size)
{
    uint16_t timeout = 0;
    uint16_t len = 0;

    memset(response, 0, response_size);

    uint16_t cmd_len = strlen(cmd);
    for(uint16_t i = 0; i < cmd_len; i++)
    {
        USART_SendData(USART1, cmd[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }

    while(timeout < 1000)
    {
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            response[len++] = USART_ReceiveData(USART1);
            if(len >= response_size - 1) break;
        }
        timeout++;
        Delay_ms(1);
    }
}

/**
 * @brief 设置AT模式
 * 注意：真正的AT模式需要手动操作HC-05模块
 */
void HC05_SetATMode(uint8_t enable)
{
    if(enable)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
        HC05.mode = HC05_MODE_AT;
        Delay_ms(100);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);
        if(HC05.connected)
            HC05.mode = HC05_MODE_CONNECTED;
        else
            HC05.mode = HC05_MODE_DISCONNECTED;
        Delay_ms(100);
    }
}

/**
 * @brief 发送数据
 */
void HC05_SendData(uint8_t *data, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        USART_SendData(USART1, data[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        Delay_ms(20);  // 增加到20ms字节间延时
    }

    // 发送完成后，额外等待数据完全传输
    Delay_ms(100);
}

/**
 * @brief 获取接收到的数据
 */
uint16_t HC05_GetData(uint8_t *buffer)
{
    uint16_t len = HC05.rx_len;

    if(len > 0)
    {
        memcpy(buffer, HC05.rx_buffer, len);
        HC05.rx_len = 0;
    }

    return len;
}

/**
 * @brief 检查蓝牙连接状态
 */
uint8_t HC05_IsConnected(void)
{
    return HC05.connected;
}

/**
 * @brief GPIO初始化
 */
static void HC05_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* 配置PA9为USART1_TX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 配置PA10为USART1_RX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 配置PA4为输出（HC-05 KEY引脚） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 初始状态：KEY拉低 */
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);

    /* 初始化USART1为9600波特率 */
    HC05_USART_Init(9600);
}

/**
 * @brief USART初始化
 */
static void HC05_USART_Init(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* USART1配置 */
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    /* 使能USART1 */
    USART_Cmd(USART1, ENABLE);

    /* 配置中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 使能接收中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

/**
 * @brief HC-05内部中断处理函数
 */
void HC05_USART_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        /* 读取接收到的数据 */
        uint8_t data = USART_ReceiveData(USART1);

        /* 存入缓冲区 */
        if(HC05.rx_len < sizeof(HC05.rx_buffer) - 1)
        {
            HC05.rx_buffer[HC05.rx_len++] = data;
        }

        /* 收到数据说明蓝牙已连接 */
        if(!HC05.connected)
        {
            HC05.connected = 1;
            HC05.mode = HC05_MODE_CONNECTED;
        }
    }
}

/**
 * @brief 检测蓝牙连接状态
 */
void HC05_CheckConnection(void)
{
    static uint8_t first_connection = 0;
    static uint32_t last_data_time = 0;

    /* 如果收到过数据，认为已连接 */
    if(HC05.rx_len > 0)
    {
        first_connection = 1;
        last_data_time = 0;
        if(!HC05.connected)
        {
            HC05.connected = 1;
            HC05.mode = HC05_MODE_CONNECTED;
        }
    }

    /* 保持连接策略 */
    if(HC05.connected)
    {
        last_data_time++;

        /* 超过60秒无数据才认为断开连接 */
        if(last_data_time > 1200)
        {
            HC05.connected = 0;
            HC05.mode = HC05_MODE_DISCONNECTED;
            first_connection = 0;
            last_data_time = 0;
        }
    }
}