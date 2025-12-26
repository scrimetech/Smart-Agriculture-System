#ifndef __CONTROL_H
#define __CONTROL_H

#include <stdint.h>

#define DEV_PUMP 1
#define DEV_FAN  2
#define DEV_LAMP 3

void Control_Init(void);
void Control_SetDevice(uint8_t deviceId, uint8_t state); // 1 = ON, 0 = OFF

#endif
