#ifndef __WATERPUMP_H
#define __WATERPUMP_H

// 抽水泵控制宏
#define WATERPUMP_ON()     GPIO_SetBits(WATERPUMP_PORT, WATERPUMP_PIN)
#define WATERPUMP_OFF()    GPIO_ResetBits(WATERPUMP_PORT, WATERPUMP_PIN)

// 函数声明
extern void WaterPump_Init(void);
extern void WaterPump_Control(uint8_t state);

#endif