#ifndef COMUNICACION_UART_H
#define COMUNICACION_UART_H

#include <stdint.h>

void    comunicacion_init(void);
void    comunicacion_enviarTexto(const char *texto);
uint8_t comunicacion_hayComando(void);              // 1 si llegó un comando completo
void    comunicacion_leerComando(char *destino);    // lo copia y deja lugar para el próximo

#endif
