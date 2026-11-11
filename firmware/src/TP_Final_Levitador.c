/*
 * Control de altura de la pelota con PID.
 * La altura se mide en mm desde la rejilla, con la tabla de calibración del sensor
 * (sensor_leerMm). El rango útil del Sharp en este tubo es de 100 a 300 mm.
 */
#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include "sensor.h"
#include "demora.h"
#include "perturbacion.h"
#include <stdio.h>
#include <stdlib.h>

#define TS_MS        40      // período del lazo: 40 ms (lo que tarda el Sharp en actualizar)
#define TELEM_MS     200     // una línea cada 200 ms: a 9600 baudios cada línea tarda ~33 ms en salir

#define U_MIN        150     // límites del duty en modo automático (por mil)
#define U_MAX        650

#define SP_MIN       100     // límites del setpoint [mm]: fuera de este rango el sensor no mide bien
#define SP_MAX       300

// Parámetros del controlador (se pueden cambiar por UART).
// Valores ajustados en el equipo el 11/11/2026 (ensayo 16:18: ±6,8 mm en régimen,
// recuperación de una perturbación de 5 cm en ~5 s).
static float    Kp = 0.300f;    // por mil de duty por mm de error
static float    Ki = 0.030f;    // por mil por (mm · s)
static float    Kd = 0.360f;    // por mil · s por mm
static float    Tf = 0.10f;     // filtro de la derivada [s]: con 0,10 el ruido del sensor mueve menos el duty
static uint16_t u0 = 560;       // duty de equilibrio (por mil); se cambia con el comando U
static uint16_t setpoint = 175; // altura deseada [mm desde la rejilla]
static uint16_t rampa = 20;     // velocidad con que se mueve el setpoint [mm/s]; 0 = salto directo
static float    spActual = 175; // setpoint que sigue el PID: avanza hacia "setpoint" a la velocidad "rampa"

static uint8_t automatico = 0;  // 0 = manual (Dxxx), 1 = PID
static float   integral = 0;
static float   derivada = 0;
static float   yAnterior = 0;

static void mostrarAyuda(void) {
    comunicacion_enviarTexto("\r\nComandos:\r\n");
    comunicacion_enviarTexto("  A      arranca el PID\r\n");
    comunicacion_enviarTexto("  Dxxx   modo manual con duty xxx (0-1000)\r\n");
    comunicacion_enviarTexto("  Sxxx   setpoint en mm (100-300)\r\n");
    comunicacion_enviarTexto("  Pxxx   Kp x1000   Ixxx  Ki x1000   Kxxx  Kd x1000\r\n");
    comunicacion_enviarTexto("  Uxxx   duty de equilibrio u0\r\n");
    comunicacion_enviarTexto("  Rxx    rampa del setpoint en mm/s (0 = salto)\r\n");
    comunicacion_enviarTexto("  Wxx    perturbacion senoidal (DAC), amplitud en por mil (0 = apagada)\r\n");
    comunicacion_enviarTexto("  Fxx    frecuencia de la perturbacion en decimas de Hz (4-50)\r\n");
    comunicacion_enviarTexto("  ?      parametros actuales\r\n");
}

static void mostrarParametros(void) {
    char texto[128];
    sprintf(texto, "SP=%u u0=%u Kp=%d Ki=%d Kd=%d (x1000) W=%u F=%u R=%u modo=%s\r\n",
            setpoint, u0, (int)(Kp * 1000), (int)(Ki * 1000), (int)(Kd * 1000),
            perturbacion_getAmplitud(), perturbacion_getFrecuencia(),
            rampa, automatico ? "AUTO" : "MANUAL");
    comunicacion_enviarTexto(texto);
}

static void arrancarPID(void) {
    uint16_t dutyActual = ventilador_getDuty();
    uint16_t yAhora     = sensor_leerMm();

    yAnterior   = yAhora;
    integral    = 0;          // u0 ya aporta el duty de equilibrio
    derivada    = 0;

    // Con rampa, el setpoint arranca desde la altura actual y avanza de a poco hasta el pedido
    spActual = (rampa > 0) ? yAnterior : setpoint;
    if (spActual < SP_MIN) spActual = SP_MIN;
    if (spActual > SP_MAX) spActual = SP_MAX;

    if (dutyActual == 0) {             // empujón suave para que el motor arranque sin tirar la pelota arriba
        ventilador_setDuty(u0 + 150 > 1000 ? 1000 : u0 + 150);
        demora_ms(300);
    }
    ventilador_setDuty(u0);
    automatico = 1;
    comunicacion_enviarTexto("PID en marcha\r\n");
    mostrarParametros();
}

// Mueve spActual hacia el setpoint pedido, como máximo "rampa" mm por segundo
static void actualizarRampa(float Ts) {
    float paso = rampa * Ts;

    if (rampa == 0) {
        spActual = setpoint;
    } else if (spActual < setpoint) {
        spActual += paso;
        if (spActual > setpoint) spActual = setpoint;
    } else if (spActual > setpoint) {
        spActual -= paso;
        if (spActual < setpoint) spActual = setpoint;
    }
}

// Un paso del PID. Se llama cada TS_MS.
static void pasoPID(float y) {
    const float Ts = TS_MS / 1000.0f;

    actualizarRampa(Ts);
    float e = spActual - y;

    // Derivada sobre la medición, filtrada (no salta al cambiar el setpoint)
    derivada = (Tf * derivada - Kd * (y - yAnterior)) / (Tf + Ts);
    yAnterior = y;

    float u = u0 + Kp * e + integral + derivada;

    // Perturbación externa generada con el DAC: se suma al duty para probar el rechazo del lazo
    // u += perturbacion_getAmplitud() * perturbacion_valor();   // DAC desactivado temporalmente

    // Saturación
    float uSat = u;
    if (uSat < U_MIN) uSat = U_MIN;
    if (uSat > U_MAX) uSat = U_MAX;

    // Anti-windup: el integrador solo acumula si la salida no está saturada
    if (u == uSat) {
        integral += Ki * Ts * e;
    }

    ventilador_setDuty((uint16_t)uSat);
}

static void atenderComando(void) {
    char cmd[16];
    char texto[48];
    int valor;

    comunicacion_leerComando(cmd);
    valor = atoi(&cmd[1]);

    switch (cmd[0]) {
        case 'A': case 'a':
            arrancarPID();
            break;
        case 'D': case 'd':
            automatico = 0;
            ventilador_setDuty(valor);
            sprintf(texto, "MANUAL duty=%u\r\n", ventilador_getDuty());
            comunicacion_enviarTexto(texto);
            break;
        case 'S': case 's':
            if (valor < SP_MIN) valor = SP_MIN;   // fuera de este rango el sensor no mide bien
            if (valor > SP_MAX) valor = SP_MAX;
            setpoint = valor;
            if (!automatico || rampa == 0) {
                spActual = setpoint;      // sin PID andando, no hace falta rampa
            }
            mostrarParametros();
            break;
        case 'P': case 'p':
            Kp = valor / 1000.0f;
            mostrarParametros();
            break;
        case 'I': case 'i':
            Ki = valor / 1000.0f;
            mostrarParametros();
            break;
        case 'K': case 'k':
            Kd = valor / 1000.0f;
            mostrarParametros();
            break;
        case 'U': case 'u':
            u0 = valor;
            mostrarParametros();
            break;
        case 'R': case 'r':
            if (valor > 200) valor = 200;
            rampa = valor;
            mostrarParametros();
            break;
        case 'W': case 'w':
            perturbacion_setAmplitud(valor);
            mostrarParametros();
            break;
        case 'F': case 'f':
            perturbacion_setFrecuencia(valor);
            mostrarParametros();
            break;
        case '?':
            mostrarParametros();
            break;
        default:
            mostrarAyuda();
            break;
    }
}

int main(void) {
    char texto[64];
    uint32_t ultimoPaso = 0;
    uint32_t ultimaTelem = 0;
    uint16_t y = 0;

    ventilador_init();
    comunicacion_init();
    sensor_init();
    // perturbacion_init();   // DAC desactivado temporalmente (prueba: ver si interfiere con el ADC)
    demora_init();
    ventilador_setDuty(0);

    comunicacion_enviarTexto("\r\nControl PID de la pelota\r\n");
    mostrarAyuda();
    mostrarParametros();

    while (1) {
        uint32_t ahora = demora_millis();

        // Lazo de control cada TS_MS
        if (ahora - ultimoPaso >= TS_MS) {
            ultimoPaso = ahora;
            y = sensor_leerMm();
            if (automatico) {
                pasoPID(y);
            }
        }

        // Telemetría: tiempo, setpoint, medición, duty
        if (ahora - ultimaTelem >= TELEM_MS) {
            ultimaTelem = ahora;
            sprintf(texto, "t=%lu sp=%u y=%u u=%u\r\n",
                    ahora, (uint16_t)(spActual + 0.5f), y, ventilador_getDuty());
            comunicacion_enviarTexto(texto);
        }

        if (comunicacion_hayComando()) {
            atenderComando();
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Versiones anteriores del main (desactivadas con #if 0)
 * ------------------------------------------------------------------------- */
#if 0
/* prueba del LED (P0.22), para verificar que la placa graba bien */
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"

#define LED_PIN  (1UL << 22)      // LED rojo de la LPCXpresso: P0.22

static void esperar(volatile uint32_t n) {
    while (n--) { }
}

int main(void) {
    // P0.22 ya arranca como GPIO (función 00 de PINSEL): solo hay que hacerlo salida
    GPIO_SetDir(PORT_0, LED_PIN, GPIO_OUTPUT);

    while (1) {
        GPIO_TogglePins(PORT_0, LED_PIN);   // si estaba prendido lo apaga, y al revés
        esperar(3000000);                   // ~medio segundo
    }
    return 0;
}


#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include "sensor.h"
#include "demora.h"
#include <stdio.h>
#include <stdlib.h>

#define PERIODO_MS   100    // una línea cada 100 ms (10 por segundo)
#define MUESTRAS     32     // muestras por lectura

static uint8_t registrando = 1;   // 1 = manda una línea cada PERIODO_MS

static uint32_t aMilivolts(uint16_t adc) {
    return (uint32_t)adc * 3300 / 4095;
}

static void mostrarAyuda(void) {
    comunicacion_enviarTexto("\r\nComandos:\r\n");
    comunicacion_enviarTexto("  Dxxx  duty en por mil (0-1000)\r\n");
    comunicacion_enviarTexto("  ?     estado actual\r\n");
    comunicacion_enviarTexto("  L     prende/apaga el registro continuo\r\n");
    comunicacion_enviarTexto("  M     medicion precisa (para calibrar)\r\n");
    comunicacion_enviarTexto("  H     esta ayuda\r\n");
}

static void fijarDuty(uint16_t nuevo) {
    // Si el ventilador está parado, le da un empujón para que arranque
    if (ventilador_getDuty() == 0 && nuevo > 0) {
        ventilador_setDuty(1000);
        demora_ms(300);
    }
    ventilador_setDuty(nuevo);
}

static void medicionPrecisa(void) {
    char texto[64];
    uint32_t suma = 0;
    uint16_t minimo = 4095, maximo = 0;
    uint8_t i;

    // 20 lecturas separadas 50 ms: cubre 1 segundo
    for (i = 0; i < 20; i++) {
        uint16_t v = sensor_leerPromedio(MUESTRAS);
        suma += v;
        if (v < minimo) minimo = v;
        if (v > maximo) maximo = v;
        demora_ms(50);
    }

    uint16_t prom = suma / 20;
    sprintf(texto, "MEDICION ADC=%u V=%lu mV (min=%u max=%u)\r\n",
            prom, aMilivolts(prom), minimo, maximo);
    comunicacion_enviarTexto(texto);
}

int main(void) {
    char cmd[16];
    char texto[64];
    uint32_t ultimo = 0;

    ventilador_init();
    comunicacion_init();
    sensor_init();
    demora_init();
    ventilador_setDuty(0);

    comunicacion_enviarTexto("\r\nPrueba sensor + ventilador\r\n");
    mostrarAyuda();

    while (1) {
        // 1. Registro periódico
        if (registrando && (demora_millis() - ultimo >= PERIODO_MS)) {
            ultimo = demora_millis();

            uint16_t adc = sensor_leerPromedio(MUESTRAS);
            sprintf(texto, "t=%lu duty=%u ADC=%u V=%lu\r\n",
                    ultimo, ventilador_getDuty(), adc, aMilivolts(adc));
            comunicacion_enviarTexto(texto);
        }

        // 2. Comandos
        if (comunicacion_hayComando()) {
            comunicacion_leerComando(cmd);

            switch (cmd[0]) {
                case 'D': case 'd':
                    fijarDuty(atoi(&cmd[1]));
                    sprintf(texto, "OK duty=%u\r\n", ventilador_getDuty());
                    comunicacion_enviarTexto(texto);
                    break;

                case '?': {
                    uint16_t adc = sensor_leerPromedio(MUESTRAS);
                    sprintf(texto, "duty=%u ADC=%u V=%lu mV\r\n",
                            ventilador_getDuty(), adc, aMilivolts(adc));
                    comunicacion_enviarTexto(texto);
                    break;
                }

                case 'L': case 'l':
                    registrando = !registrando;
                    comunicacion_enviarTexto(registrando ? "Registro ON\r\n"
                                                         : "Registro OFF\r\n");
                    break;

                case 'M': case 'm':
                    medicionPrecisa();
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

/*sensor+uart
#include "LPC17xx.h"
#include "ventilador.h"
#include "comunicacionUart.h"
#include "sensor.h"
#include "demora.h"
#include <stdio.h>
#include <stdlib.h>

#define PERIODO_LECTURA_MS  200    // 5 lecturas por segundo

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

            uint16_t adc = sensor_leerPromedio(32);   // promedio de 32 muestras
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
*/

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
#endif
