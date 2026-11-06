#include "sensor.h"
#include "lpc17xx_adc.h"

void sensor_init(void) {
    ADC_Init(200000);                  // 200 kHz: la velocidad máxima del ADC
    ADC_PinConfig(ADC_CHANNEL_0);      // P0.23 = AD0.0, sin pull-up
    ADC_ChannelEnable(ADC_CHANNEL_0);  // vamos a convertir el canal 0
}

uint16_t sensor_leer(void) {
    ADC_StartCmd(ADC_START_NOW);                                     // arranca una conversión
    while (!ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE)) { }  // espera a que termine
    return ADC_ChannelGetData(ADC_CHANNEL_0);                        // 12 bits: 0 a 4095
}
