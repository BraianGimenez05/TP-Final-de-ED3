#include "comunicacionUart.h"
#include "lpc17xx_uart.h"
#include <string.h>

void comunicacion_init(void) {
    UART_CFG_T cfg = {
        .baudRate = 115200,
        .parity   = UART_PARITY_NONE,
        .dataBits = UART_DBITS_8,
        .stopBits = UART_STOPBIT_1
    };

    UART_FIFO_CFG_T fifo = {
        .resetRxBuf = ENABLE,
        .resetTxBuf = ENABLE,
        .dmaMode    = DISABLE,
        .level      = UART_FIFO_TRGLEV0
    };

    UART_PinConfig(UART_TX0_P0_2);
    UART_PinConfig(UART_RX0_P0_3);
    UART_Init(UART0, &cfg);
    UART_FIFOConfig(UART0, &fifo);   // UART_Init deja la FIFO apagada
    UART_TxEnable(UART0);            // UART_Init deja el TX apagado
}

void comunicacion_enviarTexto(const char *texto) {
    UART_Send(UART0, (const uint8_t *)texto, strlen(texto), BLOCKING);
}
