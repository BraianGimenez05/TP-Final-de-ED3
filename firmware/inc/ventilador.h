#ifndef VENTILADOR_H
#define VENTILADOR_H

#include <stdint.h>

void     ventilador_init(void);
void     ventilador_setDuty(uint16_t permil);   // 0 a 1000 = 0,0 % a 100,0 %
uint16_t ventilador_getDuty(void);

#endif
