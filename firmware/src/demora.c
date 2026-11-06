#include "demora.h"
#include "lpc17xx_timer.h"

void demora_init(void) {
    TIM_TIMERCFG_T cfg = {
        .prescaleOpt   = TIM_US,   // el prescaler se calcula en microsegundos
        .prescaleValue = 1         // el contador suma 1 cada 1 µs
    };

    TIM_InitTimer(LPC_TIM0, &cfg);
    TIM_Enable(LPC_TIM0);          // queda contando siempre
}

uint32_t demora_millis(void) {
    return TIM_ReadTimer(LPC_TIM0) / 1000;   // el contador está en µs
}

void demora_us(uint32_t us) {
    uint32_t inicio = TIM_ReadTimer(LPC_TIM0);

    // La resta entre uint32_t sigue siendo correcta aunque el contador dé la vuelta
    while ((TIM_ReadTimer(LPC_TIM0) - inicio) < us) { }
}

void demora_ms(uint32_t ms) {
    demora_us(ms * 1000);
}
