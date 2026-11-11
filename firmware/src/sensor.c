#include "sensor.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"

#define N_MUESTRAS   64        // ~43 ms de señal a ~1500 muestras/s
#define CANAL_DMA    GPDMA_CH_0

// El DMA copia acá el registro ADDR0 completo: el resultado está en los bits 4 a 15.
// El buffer y la LLI van en la RAM2 (AHB SRAM, 0x2007C000): la RAM principal (0x10000000)
// la usa MCUXpresso para cargar el flash driver al grabar, y el DMA la pisaría.
__attribute__((section(".bss.$RAM2"))) static volatile uint32_t buffer[N_MUESTRAS];
__attribute__((section(".bss.$RAM2"))) static GPDMA_LLI_T lli;   // hace que el buffer sea circular

void sensor_init(void) {
    // 1. ADC: canal 0 (P0.23), modo burst, lo más lento posible
    ADC_Init(1500);                       // ~1500 muestras/s (divisor del reloj al máximo)
    ADC_PinConfig(ADC_CHANNEL_0);         // P0.23 = AD0.0, sin pull-up
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_IntDisable(ADC_INT_GLOBAL);       // viene habilitado de fábrica: hay que sacarlo
    ADC_IntEnable(ADC_INT_CH0);           // el canal 0 le pide datos al DMA (NO se habilita en el NVIC)

    // 2. Lista enlazada: al terminar las N_MUESTRAS vuelve a empezar en el mismo buffer
    lli.srcAddr = (uint32_t)&LPC_ADC->ADDR0;   // leer ADDR0 apaga el pedido de DMA del canal 0
    lli.dstAddr = (uint32_t)buffer;
    lli.nextLLI = (uint32_t)&lli;         // apunta a sí misma: buffer circular
    lli.control = GPDMA_DMACCxControl_TransferSize(N_MUESTRAS)
                | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
                | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
                | GPDMA_DMACCxControl_DI;  // el destino avanza; el origen (ADDR0) queda fijo

    // 3. Canal de DMA: del periférico (ADC) a la memoria (buffer)
    GPDMA_Channel_CFG_T cfg = {
        .channelNum   = CANAL_DMA,
        .transferSize = N_MUESTRAS,
        .type         = GPDMA_P2M,
        .srcMemAddr   = 0,                // no se usa: el origen es un periférico
        .dstMemAddr   = (uint32_t)buffer,
        .srcConn      = GPDMA_ADC,        // el ADC pide cada transferencia
        .dstConn      = 0,                // no se usa: el destino es memoria
        .src = { .width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = DISABLE },
        .dst = { .width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = ENABLE },
        .intTC        = DISABLE,          // no hace falta interrupción: el buffer se renueva solo
        .intErr       = DISABLE,
        .linkedList   = (uint32_t)&lli
    };

    GPDMA_Init();
    GPDMA_SetupChannel(&cfg);
    // El driver toma como origen el registro ADGDR, pero el pedido de DMA del canal 0 se apaga
    // recién al leer ADDR0. Si se lee ADGDR, el pedido queda activo y el DMA copia sin parar.
    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)&LPC_ADC->ADDR0;
    GPDMA_ChannelStart(CANAL_DMA);

    // 4. Recién ahora arranca el ADC a convertir sin parar
    ADC_BurstEnable();
}

uint16_t sensor_leer(void) {
    uint32_t suma = 0;
    uint32_t validas = 0;
    uint32_t i;

    for (i = 0; i < N_MUESTRAS; i++) {
        uint32_t dato = buffer[i];
        if (dato & (1UL << 31)) {             // bit DONE: la muestra es válida
            suma += (dato >> 4) & 0xFFF;      // el resultado está en los bits 4 a 15
            validas++;
        }
    }
    return validas ? (uint16_t)(suma / validas) : 0;
}

// Calibración del Sharp (11/11/2026): altura de la pelota desde la rejilla [mm] y lectura del ADC.
// Por encima de 350 mm la curva se da vuelta (zona ciega del sensor): no se usa.
#define N_PUNTOS 7
static const uint16_t calMm[N_PUNTOS]  = {   0,  100,  150,  200,  250,  300,  350 };
static const uint16_t calAdc[N_PUNTOS] = { 1356, 1566, 1797, 2040, 2287, 2525, 2693 };

uint16_t sensor_aMm(uint16_t adc) {
    uint8_t i;

    if (adc <= calAdc[0]) {
        return calMm[0];
    }
    if (adc >= calAdc[N_PUNTOS - 1]) {
        return calMm[N_PUNTOS - 1];
    }
    // Busca el tramo de la tabla que contiene la lectura e interpola en línea recta
    for (i = 1; i < N_PUNTOS; i++) {
        if (adc <= calAdc[i]) {
            return calMm[i - 1] + (uint32_t)(adc - calAdc[i - 1]) * (calMm[i] - calMm[i - 1])
                                  / (calAdc[i] - calAdc[i - 1]);
        }
    }
    return calMm[N_PUNTOS - 1];
}

uint16_t sensor_leerMm(void) {
    return sensor_aMm(sensor_leer());
}
