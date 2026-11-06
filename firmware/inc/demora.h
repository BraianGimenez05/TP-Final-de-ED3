#ifndef DEMORA_H
#define DEMORA_H

#include <stdint.h>

uint32_t demora_millis(void);
void demora_init(void);
void demora_ms(uint32_t ms);
void demora_us(uint32_t us);

#endif
