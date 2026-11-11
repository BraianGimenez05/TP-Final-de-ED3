#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void     sensor_init(void);
uint16_t sensor_leer(void);           // promedio de las últimas 64 muestras (ADC + DMA): 0 a 4095
uint16_t sensor_aMm(uint16_t adc);    // convierte una lectura del ADC a altura en mm (tabla de calibración)
uint16_t sensor_leerMm(void);         // altura de la pelota desde la rejilla, en mm (0 a 350)

#endif
