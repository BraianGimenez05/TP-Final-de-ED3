#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"

static void esperar(volatile uint32_t n) { while (n--) { } }

int main(void) {
    ventilador_init();
    comunicacion_init();

    ventilador_setDuty(1000);
    esperar(5000000);
    ventilador_setDuty(710);

    while (1) {
        comunicacion_enviarTexto("Hola desde la LPC\r\n");
        esperar(10000000);
    }
    return 0;
}
