#include "stm32f10x.h"

void ADC_Sensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef  ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

static u16 Get_ADC_Val(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

static u16 Get_ADC_Average(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;
    for(t=0; t<times; t++)
    {
        temp_val += Get_ADC_Val(ch);
    }
    return temp_val / times;
}

// 土壤：通道0
u8 Soil_Get_Percent(void)
{
    u16 adc = Get_ADC_Average(ADC_Channel_0, 10);
    return (4095 - adc) * 100 / 4095;
}

// 光敏：通道1
u8 Light_Get_Percent(void)
{
    u16 adc = Get_ADC_Average(ADC_Channel_1, 10);
    return (4095 - adc) * 100 / 4095;
}


//		while(1)
//    {
//        // 获取百分比
//        soil_percent  = Soil_Get_Percent();
//        light_percent = Light_Get_Percent();

//        // ========== OLED 显示数值 ==========
//        // 第1行显示土壤湿度：XX%
//        OLED_ShowNum(1, 6, soil_percent, 2);
//        OLED_ShowChar(1, 8, '%');

//        // 第2行显示光照强度：XX%
//        OLED_ShowNum(2, 7, light_percent, 2);
//        OLED_ShowChar(2, 9, '%');
//    }