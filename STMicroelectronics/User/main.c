#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "AD.h"
#include "Delay.h"
#include "WaterPump.h"
#include "CJSLDHT11.h"
#include "HC05.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

uint8_t soil_percent,light_percent;
DHT11_Data_TypeDef DHT11_Data;
uint32_t main_counter = 0;

// 非阻塞延时变量
uint32_t irrigation_counter = 0;
uint32_t bluetooth_send_counter = 0;
uint8_t needs_irrigation_check = 1;

int main(){
	OLED_Init();
	ADC_Sensor_Init();
	DHT11_Init();
	WaterPump_Init();
	HC05_Init();  // 初始化HC-05蓝牙模块

	while(1)
	{
		/* ========== 智能灌溉和传感器检测 (每5秒) ========== */
		if(main_counter >= 500)  // 5秒到了
		{
			main_counter = 0;

			// 获取百分比
	        soil_percent  = Soil_Get_Percent();
	        light_percent = Light_Get_Percent();

			OLED_Clear();
	        // ========== OLED 显示数值 ==========
	        OLED_ShowString(1, 1, "soil:");
	        OLED_ShowNum(1,6, soil_percent, 2);
	        OLED_ShowChar(1,8, '%');

	        OLED_ShowString(2, 1, "light:");
	        OLED_ShowNum(2,7, light_percent, 2);
	        OLED_ShowChar(2,9, '%');

	        /* 读取DHT11温湿度数据 */
	        uint8_t result = DHT11_Read(&DHT11_Data);

			if(result == 1)
			{
				OLED_ShowString(3, 1, "ACK Timeout");
			}
			else
			{
				OLED_ShowString(3, 1, "T:");
	            OLED_ShowNum(3, 3, DHT11_Data.temp_int+12, 2);
				OLED_ShowChar(3, 5, '.');
	            OLED_ShowNum(3, 6, DHT11_Data.temp_dec+7, 1);
				OLED_ShowChar(3, 7, 'C');

				OLED_ShowString(4, 1, "H:");
	            OLED_ShowNum(4, 3, DHT11_Data.humi_int+11, 2);
				OLED_ShowChar(4, 5, '.');
	            OLED_ShowNum(4, 6, DHT11_Data.humi_dec+9, 1);
				OLED_ShowChar(4, 7, '%');
			}

			/* ========== 智能灌溉决策算法 ========== */
			int16_t air_humidity_factor = 0;
			if (result == 0) {
				if (DHT11_Data.humi_int < 30) {
					air_humidity_factor = 15;
				} else if (DHT11_Data.humi_int < 50) {
					air_humidity_factor = 8;
				} else if (DHT11_Data.humi_int > 70) {
					air_humidity_factor = -8;
				}
			}

			int16_t temp_factor = 0;
			if (result == 0) {
				if (DHT11_Data.temp_int > 35) {
					temp_factor = 12;
				} else if (DHT11_Data.temp_int > 30) {
					temp_factor = 8;
				} else if (DHT11_Data.temp_int < 10) {
					temp_factor = -8;
				}
			}

			int16_t light_factor = 0;
			if (light_percent > 70) {
				light_factor = 8;
			} else if (light_percent < 20) {
				light_factor = -5;
			}

			uint8_t soil_threshold = 40 + air_humidity_factor + temp_factor + light_factor;
			if (soil_threshold < 25) soil_threshold = 25;
			if (soil_threshold > 55) soil_threshold = 55;

			// 判断是否需要浇水
			if (soil_percent < soil_threshold) {
				uint8_t water_deficit = soil_threshold - soil_percent;
				uint16_t water_time = 500;

				if(water_deficit > 15) {
					water_time = 1000;
				} else if(water_deficit < 5) {
					water_time = 300;
				}

				OLED_ShowEmoji(2, 3, EMOJI_CRY);

				WaterPump_Control(1);
				Delay_ms(water_time);
				WaterPump_Control(0);
			} else {
				OLED_ShowEmoji(2, 3, EMOJI_SMILE);
			}

			/* ========== 蓝牙数据发送 (同步，5秒一次) ========== */
			// 检测蓝牙连接状态
			HC05_CheckConnection();

			// 发送hex格式的传感器数据
			uint8_t bluetooth_data[4];
			bluetooth_data[0] = DHT11_Data.temp_int + 12;
			bluetooth_data[1] = DHT11_Data.humi_int + 11 - 100;  // 修正偏移量
			bluetooth_data[2] = soil_percent;
			bluetooth_data[3] = light_percent;

			HC05_SendData(bluetooth_data, 4);
		}

		/* ========== 蓝牙指令接收 (每次循环都检查) ========== */
		uint8_t rx_buffer[256];
		uint16_t rx_len = HC05_GetData(rx_buffer);

		if(rx_len > 0)
		{
			rx_buffer[rx_len] = '\0';

			// 转换为大写进行指令匹配
			for(uint16_t i = 0; i < rx_len; i++)
			{
				if(rx_buffer[i] >= 'a' && rx_buffer[i] <= 'z')
					rx_buffer[i] = rx_buffer[i] - 32;
			}

			// 解析指令
			if(strstr((char*)rx_buffer, "WATER_NOW") != NULL)
			{
				// 立即浇水
				WaterPump_Control(1);
				Delay_ms(1000);
				WaterPump_Control(0);

				// 发送hex确认
				uint8_t ack_data[1] = {0x01};  // 0x01表示浇水完成
				HC05_SendData(ack_data, 1);
			}
			else if(strstr((char*)rx_buffer, "STATUS") != NULL)
			{
				// 立即发送hex格式的状态数据
				uint8_t status_data[4];
				status_data[0] = DHT11_Data.temp_int + 12;
				status_data[1] = DHT11_Data.humi_int + 11;
				status_data[2] = soil_percent;
				status_data[3] = light_percent;
				HC05_SendData(status_data, 4);
			}
		}

		/* 短延时，保持循环快速响应 */
		Delay_ms(10);  // 10ms延时

		// 更新计数器
		main_counter++;
	}
}