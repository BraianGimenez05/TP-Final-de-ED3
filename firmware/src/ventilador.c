#include "ventilador.h"
#include "lpc17xx_pwm.h"

#define PWM_CLK_HZ   25000000U                  // PCLK del PWM = 100 MHz / 4
#define PWM_FREQ_HZ  500U                       // 500 Hz: el optoacoplador PC817 no conmuta bien más rápido
#define PWM_PERIODO  (PWM_CLK_HZ / PWM_FREQ_HZ) // 50000 cuentas = 2 ms

static uint16_t dutyActual = 0;

void ventilador_init(void) {
    PWM_TIMERCFG_T timerCfg = {
        .prescaleOpt   = PWM_TICK,
        .prescaleValue = 1             // el contador avanza 1 por cada ciclo de 25 MHz
    };
    PWM_MATCHCFG_T periodo = {         // MR0: fija el período
        .channel    = PWM_MATCH_0,
        .intEn      = DISABLE,
        .stopEn     = DISABLE,
        .resetEn    = ENABLE,          // al llegar a PWM_PERIODO, vuelve a 0
        .matchValue = PWM_PERIODO
    };
    PWM_MATCHCFG_T ancho = {           // MR1: fija cuánto tiempo está en 1
        .channel    = PWM_MATCH_1,
        .intEn      = DISABLE,
        .stopEn     = DISABLE,
        .resetEn    = DISABLE,
        .matchValue = 0                // arranca apagado
    };

    PWM_PinConfig(PWM1_P2_0);          // P2.0 pasa a ser la salida PWM1.1
    PWM_InitTimer(&timerCfg);
    PWM_ConfigMatch(&periodo);
    PWM_ConfigMatch(&ancho);
    PWM_ChannelConfig(PWM_CHANNEL_1, PWM_SINGLE_EDGE);
    PWM_ChannelEnable(PWM_CHANNEL_1);  // habilita la salida del canal 1
    PWM_ResetCounter();
    PWM_CounterEnable();               // arranca el contador
    PWM_Enable();                      // activa el modo PWM
}

void ventilador_setDuty(uint16_t permil) {
    uint32_t mr1;

    if (permil > 1000) {
        permil = 1000;
    }
    dutyActual = permil;

    if (permil == 1000) {
        mr1 = PWM_PERIODO + 1;         // MR1 > MR0: el pin nunca baja (100 %)
    } else {
        mr1 = (uint32_t)permil * PWM_PERIODO / 1000;
    }
    PWM_MatchUpdateSingle(PWM_MATCH_1, mr1);
}

uint16_t ventilador_getDuty(void) {
    return dutyActual;
}
