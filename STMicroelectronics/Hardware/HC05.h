#ifndef __HC05_H
#define __HC05_H

#include "stm32f10x.h"

/* HC-05状态枚举 */
typedef enum {
    HC05_MODE_DISCONNECTED = 0,    // 未连接
    HC05_MODE_CONNECTED = 1,       // 已连接
    HC05_MODE_AT = 2               // AT模式
} HC05_Mode;

/* HC-05配置结构体 */
typedef struct {
    uint8_t initialized;           // 初始化标志
    uint8_t connected;             // 连接状态
    HC05_Mode mode;                // 当前模式
    uint8_t rx_buffer[256];        // 接收缓冲区
    uint16_t rx_len;               // 接收长度
    uint8_t tx_buffer[256];        // 发送缓冲区
} HC05_TypeDef;

/* 函数声明 */
void HC05_Init(void);
void HC05_AT_Init(void);
void HC05_SendData(uint8_t *data, uint16_t len);
uint16_t HC05_GetData(uint8_t *buffer);
uint8_t HC05_IsConnected(void);
void HC05_SetATMode(uint8_t enable);
void HC05_SendATCommand(char *cmd, char *response, uint16_t response_size);
void HC05_CheckConnection(void);  // 新增：检测蓝牙连接状态

#endif /* __HC05_H */
