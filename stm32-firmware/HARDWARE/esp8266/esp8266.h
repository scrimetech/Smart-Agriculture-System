#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>

void ESP8266_Init(void);
void ESP8266_SendString(char* str);
void ESP8266_ClearBuffer(void);
uint8_t ESP8266_GetLine(char* line, uint16_t max_len);
uint8_t ESP8266_WaitResponse(const char* expected, uint32_t timeout_ms);
uint8_t ESP8266_SendCmd(const char* cmd, const char* expected, uint32_t timeout_ms);

#endif
