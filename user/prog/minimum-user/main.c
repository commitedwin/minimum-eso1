#include <stdint.h>

#include "minemu/user_abi.h"

#define LINE_MAX 128

static void put(const char *s, uint32_t n) {
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)s, n);
}

static void puts_str(const char *s) {
    uint32_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    put(s, n);
}

static char getc_blocking(void) {
    char c;
    while (ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)&c, 1) != 1) {
    }
    return c;
}

static char *skip_spaces(char *p) {
    while (*p == ' ') {
        p++;
    }
    return p;
}

static char *word_end(char *p) {
    while (*p != '\0' && *p != ' ') {
        p++;
    }
    return p;
}

static void cmd_echo(char *args) {
    char *p = skip_spaces(args);
    int first = 1;
    while (*p != '\0') {
        char *end = word_end(p);
        if (!first) {
            put(" ", 1);
        }
        put(p, (uint32_t)(end - p));
        first = 0;
        p = skip_spaces(end);
    }
    put("\n", 1);
}

static void run_command(char *line) {
    char *cmd = skip_spaces(line);
    if (*cmd == '\0') {
        return;
    }
    char *end = word_end(cmd);
    uint32_t len = (uint32_t)(end - cmd);

    if (len == 4 && cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o') {
        cmd_echo(end);
        return;
    }

    puts_str("command not found: ");
    put(cmd, len);
    put("\n", 1);
}

void minemu_user_main(void) {
    char line[LINE_MAX + 1];
    uint32_t len = 0;
    int too_long = 0;

    puts_str("msh> ");
    for (;;) {
        char c = getc_blocking();

        if (c == '\n') {
            if (!too_long) {
                line[len] = '\0';
                run_command(line);
            }
            len = 0;
            too_long = 0;
            puts_str("msh> ");
        } else if (c == 0x08 || c == 0x7f) {
            if (len > 0 && !too_long) {
                len--;
            }
        } else if (len < LINE_MAX) {
            line[len++] = c;
        } else {
            too_long = 1;
        }
    }
}
