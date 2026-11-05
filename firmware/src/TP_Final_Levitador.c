#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char cmd[16];
    char respuesta[32];

    ventilador_init();
    comunicacion_init();
    ventilador_setDuty(0);
    comunicacion_enviarTexto("Listo. Comandos: Dxxx (0-1000), ?\r\n");

    while (1) {
        if (comunicacion_hayComando()) {
            comunicacion_leerComando(cmd);

            if (cmd[0] == 'D' || cmd[0] == 'd') {
                ventilador_setDuty(atoi(&cmd[1]));
                sprintf(respuesta, "OK duty=%u\r\n", ventilador_getDuty());
            } else if (cmd[0] == '?') {
                sprintf(respuesta, "duty=%u\r\n", ventilador_getDuty());
            } else {
                sprintf(respuesta, "Comando desconocido\r\n");
            }
            comunicacion_enviarTexto(respuesta);
        }
    }
    return 0;
}
