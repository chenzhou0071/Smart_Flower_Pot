#ifndef __CJSLDHT11_H
#define __CJSLDHT11_H

#include "stm32f10x.h"

/* DHT11数据结构 */
typedef struct {
    uint8_t humi_int;    /* 湿度整数部分 */
    uint8_t humi_dec;    /* 湿度小数部分 */
    uint8_t temp_int;    /* 温度整数部分 */
    uint8_t temp_dec;    /* 温度小数部分 */
    uint8_t checksum;    /* 校验和 */
} DHT11_Data_TypeDef;

/* 函数声明 */
void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data_TypeDef *data);

#endif
