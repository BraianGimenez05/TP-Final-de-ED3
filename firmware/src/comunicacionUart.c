#include "comunicacionUart.h"
#include "lpc17xx_uart.h"
#include <string.h>

#define CMD_MAX 16

static char             buffer[CMD_MAX];   // donde se van juntando las letras
static uint8_t          indice = 0;        // cuántas letras hay
static volatile uint8_t comandoListo = 0;  // 1 cuando llegó el Enter

void comunicacion_init(void) {
    UART_CFG_T cfg = {
        .baudRate = 9600,
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
    UART_IntConfig(UART0, UART_INT_RBR, ENABLE);  // avisar cuando llega un dato
    NVIC_EnableIRQ(UART0_IRQn);  // permitir la interrupción en el micro
}

void comunicacion_enviarTexto(const char *texto) {
    UART_Send(UART0, (const uint8_t *)texto, strlen(texto), BLOCKING);
}

void UART0_IRQHandler(void) {
    uint32_t id = UART_GetIntId(UART0) & UART_IIR_INTID_MASK;

    if (id == UART_IIR_INTID_RDA || id == UART_IIR_INTID_CTI) {
        while (UART_GetLineStatus(UART0) & UART_LSR_RDR) {
            char c = UART_ReceiveByte(UART0);

            if (comandoListo) {
                continue;                  // el main todavía no leyó el anterior: se descarta
            }

            if (c == '\r' || c == '\n') {
                if (indice > 0) {
                    buffer[indice] = '\0'; // cierra el texto
                    comandoListo = 1;
                }
                UART_SendByte(UART0, '\r');
                UART_SendByte(UART0, '\n'); // baja de renglón en la pantalla
            } else if (indice < CMD_MAX - 1) {
                buffer[indice++] = c;
                UART_SendByte(UART0, c);    // eco
            }
        }
    }
}

uint8_t comunicacion_hayComando(void) {
    return comandoListo;
}

void comunicacion_leerComando(char *destino) {
    strcpy(destino, buffer);   // copia el comando
    indice = 0;                // vacía el buffer
    comandoListo = 0;          // deja recibir el próximo
}
