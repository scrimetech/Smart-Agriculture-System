#ifndef __SENSOR_H
#define __SENSOR_H

#include <stdint.h>

void Sensor_Init(void);
uint8_t Sensor_ReadDHT11(uint8_t* temp, uint8_t* humi);
uint16_t Sensor_ReadSoil(void);
uint16_t Sensor_ReadLight(void);

#endif
