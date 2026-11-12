/*
 * Perturbación senoidal con DAC + DMA.
 *
 * Una tabla con un período de senoidal se copia por DMA (canal 1, buffer circular)
 * al registro del DAC. El contador del DAC marca cada cuánto pide la muestra siguiente,
 * y eso fija la frecuencia. La salida AOUT (P0.26) maneja el buzzer a través del
 * seguidor, y el PID lee el valor actual del DAC para sumarlo al duty del ventilador.
 *
 * Modo cuadrada (comando C): el DMA se detiene y la interrupción del Timer1 escribe el DAC
 * directamente, alternando entre el máximo y el mínimo cada medio período. Así la perturbación
 * sube y baja de golpe, con un período de 0,2 a 60 s (la senoidal con DMA no baja de 0,4 Hz
 * porque el contador del DAC es de 16 bits).
 */
#include "perturbacion.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include <math.h>

#define N_TABLA      1024           // muestras por período
#define CANAL_DMA    GPDMA_CH_1     // el canal 0 lo usa el sensor
#define PCLK_DAC     25000000UL     // DAC_Init deja el reloj del DAC en CCLK / 4
#define FREC_MIN     4              // 0,4 Hz: el contador del DAC es de 16 bits
#define FREC_MAX     50             // 5 Hz
#define CUAD_MIN     2              // 0,2 s
#define CUAD_MAX     600            // 60 s
#define DAC_MAX      1023           // salida del DAC: 0 a 1023 (10 bits)

// Tabla y lista enlazada en la RAM2, igual que el buffer del sensor
__attribute__((section(".bss.$RAM2"))) static uint32_t tabla[N_TABLA];
__attribute__((section(".bss.$RAM2"))) static GPDMA_LLI_T lli;

static uint16_t amplitud   = 0;     // por mil
static uint16_t frecuencia = 5;     // décimas de Hz (0,5 Hz)
static uint8_t  inicializado = 0;   // si perturbacion_init no se llamó, no se toca el hardware

static uint16_t periodoCuadrada = 0;   // décimas de s; 0 = modo senoidal
static uint32_t msCuadrada = 0;        // tiempo transcurrido en el medio período actual
static uint8_t  nivelAlto = 1;         // 1 = la cuadrada está arriba, 0 = abajo

static void habilitarSalida(FunctionalState estado) {
    DAC_CONVERTER_CFG_T cfg = {
        .doubleBuffer = estado,
        .dmaCounter   = estado,     // el contador marca el ritmo de las muestras
        .dmaRequest   = estado      // al vencer el contador, el DAC le pide un dato al DMA
    };
    DAC_ConfigDAConverterControl(&cfg);
}

void perturbacion_init(void) {
    uint32_t i;

    DAC_Init();                     // P0.26 como AOUT

    // 1. Tabla: un período de senoidal centrado en la mitad del rango (0 a 1023).
    //    El DAC toma el valor en los bits 6 a 15 del registro DACR.
    for (i = 0; i < N_TABLA; i++) {
        float s = sinf(2.0f * 3.14159265f * i / N_TABLA);
        uint32_t valor = (uint32_t)(512.0f + 511.0f * s);
        tabla[i] = valor << 6;
    }

    // 2. Lista enlazada que apunta a sí misma: la tabla se repite sin fin
    lli.srcAddr = (uint32_t)tabla;
    lli.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lli.nextLLI = (uint32_t)&lli;
    lli.control = GPDMA_DMACCxControl_TransferSize(N_TABLA)
                | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
                | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
                | GPDMA_DMACCxControl_SI;  // el origen avanza por la tabla; el destino (DACR) queda fijo

    // 3. Canal de DMA: de la memoria (tabla) al periférico (DAC).
    //    GPDMA_Init ya lo llamó sensor_init: llamarlo de nuevo reiniciaría el canal del sensor.
    GPDMA_Channel_CFG_T cfg = {
        .channelNum   = CANAL_DMA,
        .transferSize = N_TABLA,
        .type         = GPDMA_M2P,
        .srcMemAddr   = (uint32_t)tabla,
        .dstMemAddr   = 0,
        .srcConn      = 0,
        .dstConn      = GPDMA_DAC,   // el DAC pide cada transferencia
        .src = { .width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = ENABLE },
        .dst = { .width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = DISABLE },
        .intTC        = DISABLE,
        .intErr       = DISABLE,
        .linkedList   = (uint32_t)&lli
    };
    GPDMA_SetupChannel(&cfg);
    GPDMA_ChannelStart(CANAL_DMA);

    inicializado = 1;

    // 4. Ritmo de las muestras y salida apagada hasta que se pida una amplitud
    perturbacion_setFrecuencia(frecuencia);
    perturbacion_setAmplitud(0);
}

void perturbacion_setFrecuencia(uint16_t dHz) {
    if (dHz < FREC_MIN) dHz = FREC_MIN;
    if (dHz > FREC_MAX) dHz = FREC_MAX;
    frecuencia = dHz;
    if (!inicializado) {
        return;
    }

    // Cada muestra dura PCLK / (f · N) ciclos del reloj del DAC
    uint32_t cuenta = (PCLK_DAC * 10UL) / ((uint32_t)dHz * N_TABLA);
    DAC_SetDMATimeOut((uint16_t)cuenta);
}

void perturbacion_setAmplitud(uint16_t permil) {
    if (permil > 200) permil = 200;
    amplitud = permil;
    if (!inicializado) {
        return;
    }

    if (amplitud == 0) {
        habilitarSalida(DISABLE);   // el DMA deja de recibir pedidos
        DAC_UpdateValue(0);         // salida en 0 V: el buzzer queda en silencio
    } else if (periodoCuadrada > 0) {
        habilitarSalida(DISABLE);   // cuadrada: el DAC lo escribe el Timer1, no el DMA
        DAC_UpdateValue(nivelAlto ? DAC_MAX : 0);
    } else {
        habilitarSalida(ENABLE);
    }
}

void perturbacion_setCuadrada(uint16_t dS) {
    if (dS != 0 && dS < CUAD_MIN) dS = CUAD_MIN;
    if (dS > CUAD_MAX) dS = CUAD_MAX;
    periodoCuadrada = dS;
    msCuadrada = 0;
    nivelAlto = 1;                  // arranca con el salto hacia arriba
    perturbacion_setAmplitud(amplitud);   // reconfigura la salida para el modo elegido
}

uint16_t perturbacion_getCuadrada(void) { return periodoCuadrada; }

void perturbacion_alarma(uint8_t encendida) {
    if (!inicializado || amplitud != 0) {
        return;                     // con perturbación, la salida del DAC es de la perturbación
    }
    // Con amplitud 0 el DMA está detenido y perturbacion_valor() devuelve 0, así que escribir
    // el DAC no cambia el duty: solo prende o apaga el buzzer
    DAC_UpdateValue(encendida ? DAC_MAX : 0);
}

void perturbacion_tick(uint16_t ms) {
    if (!inicializado || periodoCuadrada == 0 || amplitud == 0) {
        return;
    }
    msCuadrada += ms;
    if (msCuadrada >= (uint32_t)periodoCuadrada * 50) {    // medio período: décimas de s × 100 / 2
        msCuadrada = 0;
        nivelAlto = !nivelAlto;
        DAC_UpdateValue(nivelAlto ? DAC_MAX : 0);           // salto de golpe
    }
}

uint16_t perturbacion_getAmplitud(void)  { return amplitud; }
uint16_t perturbacion_getFrecuencia(void) { return frecuencia; }

float perturbacion_valor(void) {
    if (amplitud == 0 || !inicializado) {
        return 0.0f;
    }
    // Lee lo que está sacando el DAC en este momento (bits 6 a 15) y lo pasa a -1 .. 1
    int32_t valor = (int32_t)((LPC_DAC->DACR >> 6) & 0x3FF);
    return (valor - 512) / 511.0f;
}
