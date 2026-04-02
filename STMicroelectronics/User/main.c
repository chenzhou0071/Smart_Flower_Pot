#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "AD.h"
#include "Delay.h"
#include "WaterPump.h"
#include "CJSLDHT11.h"

uint8_t soil_percent,light_percent;
DHT11_Data_TypeDef DHT11_Data;

int main(){
	OLED_Init();
	ADC_Sensor_Init();
	DHT11_Init();
	WaterPump_Init();
	
	while(1)
	{
		// 获取百分比
        soil_percent  = Soil_Get_Percent();
        light_percent = Light_Get_Percent();
		
		OLED_Clear();
        // ========== OLED 显示数值 ==========
        // 第1行显示土壤湿度：XX%
		OLED_ShowString(1, 1, "soil:");
        OLED_ShowNum(1,6, soil_percent, 2);
        OLED_ShowChar(1,8, '%');

        // 第2行显示光照强度：XX%
		OLED_ShowString(2, 1, "light:");
        OLED_ShowNum(2,7, light_percent, 2);
        OLED_ShowChar(2,9, '%');


		/* 读取DHT11温湿度数据 */
		uint8_t result = DHT11_Read(&DHT11_Data);

		if(result == 1)
		{
			/* 响应超时，显示错误 */
			OLED_ShowString(3, 1, "ACK Timeout");
		}
		else if(result == 0)
		{
			/* 读取成功，显示温度和湿度，带单位符号 */
			OLED_ShowString(3, 1, "T:");
			OLED_ShowNum(3, 3, DHT11_Data.temp_int, 2);
			OLED_ShowChar(3, 5, '.');
			OLED_ShowNum(3, 6, DHT11_Data.temp_dec, 1);
			OLED_ShowChar(3, 7, 'C');

			OLED_ShowString(4, 1, "H:");
			OLED_ShowNum(4, 3, DHT11_Data.humi_int, 2);
			OLED_ShowChar(4, 5, '.');
			OLED_ShowNum(4, 6, DHT11_Data.humi_dec, 1);
			OLED_ShowChar(4, 7, '%');
		}
		else if(result == 2)
		{
			/* 校验失败，显示数据但不显示单位符号 */
			OLED_ShowString(3, 1, "T:");
			OLED_ShowNum(3, 3, DHT11_Data.temp_int, 2);
			OLED_ShowChar(3, 5, '.');
			OLED_ShowNum(3, 6, DHT11_Data.temp_dec, 1);

			OLED_ShowString(4, 1, "H:");
			OLED_ShowNum(4, 3, DHT11_Data.humi_int, 2);
			OLED_ShowChar(4, 5, '.');
			OLED_ShowNum(4, 6, DHT11_Data.humi_dec, 1);
		}

		/* 延时5秒 (官方推荐的稳定间隔>=5s) */
		if(soil_percent < 50){
			WaterPump_Open();
			OLED_ShowEmoji(2,3,EMOJI_CRY);	
		}
		Delay_s(5);
	}
} 
