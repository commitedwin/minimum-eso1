#include <stdbool.h>
#include <stdint.h>

#include "minemu/platform.h"
#include "minemu/syscall.h"
#include "minemu/trap.h"
#include "minemu/user_abi.h"
#include "process.h"

#define CPSR_MODE_MASK UINT32_C(0x1f)
#define CPSR_MODE_USR UINT32_C(0x10)
#define CPSR_THUMB UINT32_C(0x20)
#define A32_SVC_ZERO UINT32_C(0xef000000)

int console_getc(char *c);

static void uart_send(volatile struct minemu_uart_regs *uart, uint8_t byte) {
    while ((uart->status & MINEMU_UART_STATUS_TX_READY) == 0) {
    }
    uart->tx_data = byte;
}

static int32_t uart_write(volatile struct minemu_uart_regs *uart, uint32_t user_buf,
                          uint32_t count) {
    uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
    if (count > MINEMU_IOCTL_MAX_TRANSFER || !copy_from_user(bytes, user_buf, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    for (uint32_t i = 0; i < count; i++) {
        uart_send(uart, bytes[i]);
    }
    return (int32_t)count;
}

static int32_t uart_read(uint32_t user_buf, uint32_t count) {
    uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
    if (count > MINEMU_IOCTL_MAX_TRANSFER) {
        return MINEMU_IOCTL_ERROR;
    }
    uint32_t got = 0;
    char c;
    while (got < count && console_getc(&c)) {
        bytes[got++] = (uint8_t)c;
    }
    if (!copy_to_user(user_buf, bytes, got)) {
        return MINEMU_IOCTL_ERROR;
    }
    return (int32_t)got;
}

static int32_t rng_read(uint32_t request, uint32_t user_out) {
    uint32_t value = request == MINEMU_IOCTL_RNG_NEXT ? MINEMU_RNG->data
                                                      : MINEMU_RNG->state;
    if (!copy_to_user(user_out, &value, sizeof(value))) {
        return MINEMU_IOCTL_ERROR;
    }
    return 0;
}

static int32_t sys_ioctl(uint32_t fd, uint32_t request, uint32_t arg0, uint32_t arg1) {
    switch (request) {
    case MINEMU_IOCTL_UART_READ:
        if (fd != MINEMU_FD_UART_IN) {
            return MINEMU_IOCTL_ERROR;
        }
        return uart_read(arg0, arg1);

    case MINEMU_IOCTL_UART_WRITE:
        if (fd == MINEMU_FD_UART_OUT) {
            return uart_write(MINEMU_UART0, arg0, arg1);
        }
        if (fd == MINEMU_FD_UART_DIAG) {
            return uart_write(MINEMU_UART1, arg0, arg1);
        }
        return MINEMU_IOCTL_ERROR;

    case MINEMU_IOCTL_RNG_NEXT:
    case MINEMU_IOCTL_RNG_STATE:
        if (fd != MINEMU_FD_RNG) {
            return MINEMU_IOCTL_ERROR;
        }
        return rng_read(request, arg0);

    case MINEMU_IOCTL_RNG_SEED:
        if (fd != MINEMU_FD_RNG) {
            return MINEMU_IOCTL_ERROR;
        }
        MINEMU_RNG->seed = arg0;
        return 0;

    case MINEMU_IOCTL_TRACE_EVENT:
        if (fd != MINEMU_FD_TRACE) {
            return MINEMU_IOCTL_ERROR;
        }
        MINEMU_TRACE->event = arg0;
        return 0;

    default:
        return MINEMU_IOCTL_ERROR;
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    uint32_t instruction = 0;
    if (frame->exception_id != MINEMU_EXCEPTION_SVC ||
        (frame->spsr & CPSR_MODE_MASK) != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0 ||
        !copy_from_user(&instruction, frame->fault_pc, sizeof(instruction)) ||
        instruction != A32_SVC_ZERO) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    if (frame->r[7] == MINEMU_SYSCALL_IOCTL) {
        frame->r[0] = (uint32_t)sys_ioctl(frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
    } else {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
    }
    return frame;
}
