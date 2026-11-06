#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void     sensor_init(void);
uint16_t sensor_leer(void);   // valor crudo del ADC: 0 a 4095

#endif
