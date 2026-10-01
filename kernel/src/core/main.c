#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"

void uart_puts(const char *s);
void interrupts_init(void);
int console_getc(char *c);

static void run_command(char *line) {
    char *p = line;

    while (*p == ' ') p++;
    if (*p == '\0') return;

    if (p[0] == 'e' && p[1] == 'c' && p[2] == 'h' && p[3] == 'o' &&
        (p[4] == ' ' || p[4] == '\0')) {
        p += 4;
        while (*p == ' ') p++; 
        uart_puts(p);
        uart_puts("\n");
        return;
    }

    char *end = p;
    while (*end != '\0' && *end != ' ') end++;
    *end = '\0';
    uart_puts("command not found: ");
    uart_puts(p);
    uart_puts("\n");
}

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    minemu_trace_event(1);
    interrupts_init();

    char line[21];
    int len = 0;
    int too_long = 0;

    uart_puts("msh> ");

    for (;;) {
        char c;
        if (!console_getc(&c)) {
            continue;
        }

        if (c == '\n') {
            if (too_long) {
                uart_puts("error: max 20 bytes\n");
            } else {
                line[len] = '\0';
                run_command(line);
            }
            len = 0;
            too_long = 0;
            uart_puts("msh> ");
        } else if (c == 0x08 || c == 0x7f) {
            if (len > 0) {
                len--;
            }
        } else if (len < 20) {
            line[len++] = c;
        } else {
            too_long = 1;
        }
    }
}