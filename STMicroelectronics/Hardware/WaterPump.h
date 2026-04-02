#ifndef __WATERPUMP_H
#define __WATERPUMP_H

// ��ˮ�ÿ��ƺ�
#define WATERPUMP_ON()     GPIO_SetBits(WATERPUMP_PORT, WATERPUMP_PIN)
#define WATERPUMP_OFF()    GPIO_ResetBits(WATERPUMP_PORT, WATERPUMP_PIN)

// ��������
extern void WaterPump_Init(void);
extern void WaterPump_Control(uint8_t state);
void WaterPump_Open();

#endif