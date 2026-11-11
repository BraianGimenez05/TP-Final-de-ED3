#ifndef PERTURBACION_H
#define PERTURBACION_H

#include <stdint.h>

void     perturbacion_init(void);                  // llamar después de sensor_init (usa el GPDMA ya inicializado)
void     perturbacion_setAmplitud(uint16_t permil);  // amplitud que se suma al duty, en por mil (0 = apagada)
void     perturbacion_setFrecuencia(uint16_t dHz);   // frecuencia en décimas de Hz (4 a 50 = 0,4 a 5 Hz)
uint16_t perturbacion_getAmplitud(void);
uint16_t perturbacion_getFrecuencia(void);
float    perturbacion_valor(void);                 // valor actual de la senoidal, de -1 a 1 (0 si está apagada)

#endif
