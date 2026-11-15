#ifndef SALIDA_ALTURA_H
#define SALIDA_ALTURA_H

#include <stdint.h>

// Salida analógica de la altura por el DAC (AOUT, P0.26): 0 a 350 mm -> 0 a 3,3 V
void salidaAltura_init(void);
void salidaAltura_escribir(uint16_t mm);

#endif
