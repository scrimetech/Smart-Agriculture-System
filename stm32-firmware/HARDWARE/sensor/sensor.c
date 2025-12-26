#include "sensor.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

// DHT11: PA5
// Soil ADC: PA1 (ADC1 通道1)
// 光敏电阻: PA4 (ADC1 通道4)

static void Safe_Delay_us(uint32_t us) 
{
    volatile uint32_t count = us * 12;
    while(count--);
}

void ADC_Configuration(void) 
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72MHz / 6 = 12MHz

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4;
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

void Sensor_Init(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOA, GPIO_Pin_5);
    
    ADC_Configuration();
}

void DHT11_IO_OUT(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DHT11_IO_IN(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint8_t Sensor_ReadDHT11(uint8_t* temp, uint8_t* humi) 
{
    uint8_t buf[5] = {0};
    uint8_t i, j;
    uint8_t retry = 0;

    DHT11_IO_OUT();
    GPIO_ResetBits(GPIOA, GPIO_Pin_5);
    vTaskDelay(20); 
    GPIO_SetBits(GPIOA, GPIO_Pin_5);
    Safe_Delay_us(30);

    DHT11_IO_IN();
    
    retry = 0;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == SET && retry < 100) 
	{ 
		retry++; 
		Safe_Delay_us(1); 
	}
	
    if (retry >= 100) 
		return 0;
    
    retry = 0;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == RESET && retry < 100) 
	{ 
		retry++; 
		Safe_Delay_us(1); 
	}
	
    if (retry >= 100) 
		return 0;
    
    retry = 0;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == SET && retry < 100) 
	{
		retry++; 
		Safe_Delay_us(1); 
	}
	
    if (retry >= 100) 
		return 0;

    taskENTER_CRITICAL(); 
    for (i = 0; i < 5; i++) 
    {
        for (j = 0; j < 8; j++) 
        {
            retry = 0;
            while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == RESET && retry < 100) 
			{ 
				retry++; 
				Safe_Delay_us(1); 
			}

            uint32_t high_time = 0;
            while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == SET && high_time < 1000) 
            {
                high_time++;
            }
            
            buf[i] <<= 1;

            if (high_time > 100) 
            {
                buf[i] |= 1;
            }
        }
    }
    taskEXIT_CRITICAL();

    if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4]) 
    {
        *humi = buf[0];
        *temp = buf[2];
        return 1;
    }
    return 0;
}

uint16_t Sensor_ReadSoil(void) 
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    uint16_t adc_val = ADC_GetConversionValue(ADC1);
    if(adc_val > 4095) 
		adc_val = 4095;
    uint8_t percent = 100 - (adc_val * 100 / 4095);
    return 
		percent;
}

uint16_t Sensor_ReadLight(void) 
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    uint16_t adc_val = ADC_GetConversionValue(ADC1);
    if(adc_val > 4095) 
		adc_val = 4095;

    return (uint16_t)(100 - (adc_val * 100 / 4095));
}

