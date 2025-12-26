#include "control.h"
#include "stm32f10x.h"

#define PUMP_PIN   GPIO_Pin_12
#define FAN_PIN    GPIO_Pin_13
#define LAMP_PIN   GPIO_Pin_14
#define RELAY_PORT GPIOB

void Control_Init(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = PUMP_PIN | FAN_PIN | LAMP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RELAY_PORT, &GPIO_InitStructure);
    
    GPIO_ResetBits(RELAY_PORT, PUMP_PIN | FAN_PIN | LAMP_PIN);
}

void Control_SetDevice(uint8_t deviceId, uint8_t state) 
{
    uint16_t pin = 0;
    
    switch(deviceId) 
	{
        case DEV_PUMP: pin = PUMP_PIN; break;
        case DEV_FAN:  pin = FAN_PIN; break;
        case DEV_LAMP: pin = LAMP_PIN; break;
        default: return;
    }
    
    if (state) 
	{
        GPIO_SetBits(RELAY_PORT, pin);
    } 
	else 
	{
        GPIO_ResetBits(RELAY_PORT, pin);
    }
}
