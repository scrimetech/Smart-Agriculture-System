#include "esp8266.h"
#include "stm32f10x.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"

//使用的是USART2: Tx -> PA2; Rx -> PA3

#define RX_BUF_SIZE 1024
volatile uint8_t rx_buffer[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

void USART2_IRQHandler(void) 
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 
	{
        uint8_t data = USART_ReceiveData(USART2);
        uint16_t next_head = (rx_head + 1) % RX_BUF_SIZE;
        if (next_head != rx_tail) 
		{
            rx_buffer[rx_head] = data;
            rx_head = next_head;
        }
    }
}

void ESP8266_Init(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

void ESP8266_SendString(char* str) 
{
    while (*str) 
	{
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
        USART_SendData(USART2, *str++);
    }
}

void ESP8266_ClearBuffer(void) 
{
    rx_head = rx_tail = 0;
}

uint8_t ESP8266_GetLine(char* line, uint16_t max_len) 
{
    uint16_t temp_tail = rx_tail;
    uint16_t i = 0;
    
    while (temp_tail != rx_head) 
	{
        char c = rx_buffer[temp_tail];
        temp_tail = (temp_tail + 1) % RX_BUF_SIZE;
        
        if (c == '\n') 
		{        
            while (rx_tail != temp_tail) 
			{
                char ch = rx_buffer[rx_tail];
                rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
                if (i < max_len - 1) 
				{
                    line[i++] = ch;
                }
            }
            line[i] = '\0';
            return 1;
        }
    }
    return 0;
}

uint8_t ESP8266_WaitResponse(const char* expected, uint32_t timeout_ms) 
{
    char line[256];
    uint32_t start_tick = xTaskGetTickCount();
    
    while (xTaskGetTickCount() - start_tick < timeout_ms) 
	{
        if (ESP8266_GetLine(line, 256)) 
		{
            if (strstr(line, expected)) 
			{
                return 1; 
            }
        }
        vTaskDelay(10); 
    }
    return 0; 
}

uint8_t ESP8266_SendCmd(const char* cmd, const char* expected, uint32_t timeout_ms) 
{
    ESP8266_ClearBuffer();
    ESP8266_SendString((char*)cmd);
    return ESP8266_WaitResponse(expected, timeout_ms);
}
