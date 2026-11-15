/*
 * Salida analógica de la altura con el DAC.
 *
 * En cada paso del lazo de control (interrupción del Timer1, cada 40 ms) se escribe en el DAC
 * la altura medida por el ADC, ya convertida a mm: 0 mm -> 0 V y 350 mm -> 3,3 V (~9,4 mV/mm).
 * La salida AOUT (P0.26) pasa por un seguidor (LM358, mitad B) y se puede medir con un tester
 * o un osciloscopio.
 */
#include "salidaAltura.h"
#include "lpc17xx_dac.h"

#define DAC_MAX      1023           // el DAC es de 10 bits: 0 a 1023
#define ALTURA_MAX   350            // [mm] altura que corresponde a 3,3 V

void salidaAltura_init(void) {
    DAC_Init();                     // P0.26 como AOUT
    DAC_UpdateValue(0);
}

void salidaAltura_escribir(uint16_t mm) {
    if (mm > ALTURA_MAX) mm = ALTURA_MAX;
    DAC_UpdateValue((uint32_t)mm * DAC_MAX / ALTURA_MAX);
}
