#ifndef PERTURBACION_H
#define PERTURBACION_H

#include <stdint.h>

void     perturbacion_init(void);                  // llamar después de sensor_init (usa el GPDMA ya inicializado)
void     perturbacion_setAmplitud(uint16_t permil);  // amplitud que se suma al duty, en por mil (0 = apagada)
void     perturbacion_setFrecuencia(uint16_t dHz);   // frecuencia en décimas de Hz (4 a 50 = 0,4 a 5 Hz)
uint16_t perturbacion_getAmplitud(void);
uint16_t perturbacion_getFrecuencia(void);
float    perturbacion_valor(void);                 // valor actual de la perturbación, de -1 a 1 (0 si está apagada)

// Onda cuadrada: la perturbación salta de golpe entre +amplitud y -amplitud.
// periodo en décimas de segundo (2 a 600 = 0,2 a 60 s); 0 vuelve a la senoidal.
void     perturbacion_setCuadrada(uint16_t dS);
uint16_t perturbacion_getCuadrada(void);           // período actual en décimas de s (0 = senoidal)
void     perturbacion_tick(uint16_t ms);           // se llama desde el Timer1 cada "ms" milisegundos

// Alarma con el buzzer: con la perturbación apagada (W0), pone la salida del DAC en alto (buzzer
// sonando) o en 0 V. Con la perturbación prendida no hace nada: el DAC lo usa la perturbación.
void     perturbacion_alarma(uint8_t encendida);

#endif
