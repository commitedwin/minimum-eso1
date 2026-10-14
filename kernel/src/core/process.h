#ifndef CORE_PROCESS_H
#define CORE_PROCESS_H

#include <stdbool.h>
#include <stdint.h>

#include "minemu/boot.h"

void start_user_program(const struct minemu_boot_info *boot_info, const char *name)
    __attribute__((noreturn));
bool copy_from_user(void *dst, uint32_t user_src, uint32_t length);
bool copy_to_user(uint32_t user_dst, const void *src, uint32_t length);

#endif
