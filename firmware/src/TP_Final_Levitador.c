#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include "sensor.h"
#include "demora.h"
#include <stdio.h>
#include <stdlib.h>

#define PERIODO_LECTURA_MS  1000     // 5 lecturas por segundo

int main(void) {
    char cmd[16];
    char respuesta[40];
    uint32_t ultimaLectura = 0;

    ventilador_init();
    comunicacion_init();
    sensor_init();
    demora_init();
    ventilador_setDuty(0);
    comunicacion_enviarTexto("Listo. Comandos: Dxxx (0-1000), ?\r\n");

    while (1) {
        // 1. ¿Pasaron 200 ms desde la última lectura? Si no, sigue de largo
        if (demora_millis() - ultimaLectura >= PERIODO_LECTURA_MS) {
            ultimaLectura = demora_millis();

            uint16_t adc = sensor_leer();
            uint32_t mv  = (uint32_t)adc * 3300 / 4095;
            sprintf(respuesta, "ADC=%u  V=%lu mV\r\n", adc, mv);
            comunicacion_enviarTexto(respuesta);
        }

        // 2. Los comandos se atienden en cada vuelta, sin esperar
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

/*codigo prueba de pwm y uart
#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include "demora.h"
#include <stdio.h>
#include <stdlib.h>

static void mostrarAyuda(void) {
    comunicacion_enviarTexto("\r\nComandos:\r\n");
    comunicacion_enviarTexto("  Dxxx  duty en por mil (0-1000)\r\n");
    comunicacion_enviarTexto("  ?     duty actual y registros del PWM\r\n");
    comunicacion_enviarTexto("  R     rampa 0 -> 100 % -> 0\r\n");
    comunicacion_enviarTexto("  H     esta ayuda\r\n");
}

static void rampa(void) {
    char texto[24];
    uint16_t d;

    for (d = 0; d <= 1000; d += 100) {          // sube de a 10 %
        ventilador_setDuty(d);
        sprintf(texto, "duty=%u\r\n", d);
        comunicacion_enviarTexto(texto);
        demora_ms(500);                         // medio segundo en cada escalón
    }
    for (d = 1000; d > 0; d -= 100) {           // baja de a 10 %
        ventilador_setDuty(d - 100);
        sprintf(texto, "duty=%u\r\n", d - 100);
        comunicacion_enviarTexto(texto);
        demora_ms(500);
    }
    comunicacion_enviarTexto("Rampa terminada\r\n");
}

int main(void) {
    char cmd[16];
    char respuesta[48];

    ventilador_init();
    comunicacion_init();
    demora_init();
    ventilador_setDuty(0);

    comunicacion_enviarTexto("\r\nPrueba UART + PWM\r\n");
    mostrarAyuda();

    while (1) {
        if (comunicacion_hayComando()) {
            comunicacion_leerComando(cmd);

            switch (cmd[0]) {
                case 'D': case 'd':
                    ventilador_setDuty(atoi(&cmd[1]));
                    sprintf(respuesta, "OK duty=%u\r\n", ventilador_getDuty());
                    comunicacion_enviarTexto(respuesta);
                    break;

                case '?':
                    sprintf(respuesta, "duty=%u MR0=%lu MR1=%lu\r\n",
                            ventilador_getDuty(), LPC_PWM1->MR0, LPC_PWM1->MR1);
                    comunicacion_enviarTexto(respuesta);
                    break;

                case 'R': case 'r':
                    rampa();
                    break;

                case 'H': case 'h':
                    mostrarAyuda();
                    break;

                default:
                    comunicacion_enviarTexto("Comando desconocido (H = ayuda)\r\n");
                    break;
            }
        }
    }
    return 0;
}
*/
