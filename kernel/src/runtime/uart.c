#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h"



void uart_putc(char byte) {

//wait till tx is ready
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
        }
    MINEMU_UART0->tx_data = (uint32_t)byte;

}


void uart_puts(const char *s){
    while (*s != '\0') {
        uart_putc(*s);
        s++;
    }
}