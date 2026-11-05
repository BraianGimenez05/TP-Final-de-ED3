#include "LPC17xx.h"
#include "ventilador.h"

static void esperar(volatile uint32_t n) {
    while (n--) {
    }
}

int main(void) {
    ventilador_init();

    while (1) {
        ventilador_setDuty(300);     // 30 %
        esperar(20000000);
        ventilador_setDuty(1000);    // 100 %
        esperar(20000000);
        ventilador_setDuty(0);       // apagado
        esperar(20000000);
    }
    return 0;
}
