#include "minemu/platform.h"
#include "minemu/irq.h"
#include "minemu/trap.h"

#define BUF_SIZE 64
static volatile char buf[BUF_SIZE];
static volatile uint32_t head = 0;
static volatile uint32_t tail = 0;

static void uart_handler(void) {
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        char c = (char)(MINEMU_UART0->rx_data & 0xFF);
        if ((head + 1) % BUF_SIZE != tail) {  
            buf[head] = c;
            head = (head + 1) % BUF_SIZE;
        }
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {
    if (frame->exception_id == MINEMU_IRQ_UART0) {
        uart_handler();
    }
    MINEMU_INTERRUPT->eoi = frame->exception_id;
    return frame;
}

void interrupts_init(void) {
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE; 
    MINEMU_INTERRUPT->enable = UINT32_C(1) << MINEMU_IRQ_UART0; 
    minemu_irq_enable(); 
}

int console_getc(char *c) {
    int got = 0;
    minemu_irq_disable();
    if (tail != head) { 
        *c = buf[tail]; 
        tail = (tail + 1) % BUF_SIZE;
        got = 1;
    }
    minemu_irq_enable();
    return got;
}