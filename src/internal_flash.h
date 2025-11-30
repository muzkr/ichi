#ifndef _INTERNAL_FLASH_H
#define _INTERNAL_FLASH_H

#include <stdint.h>
#include <string.h>
#include "py32f0xx.h"

static inline void internal_flash_read_page(uint32_t addr, uint8_t *buf)
{
    memcpy(buf, (void *)addr, FLASH_PAGE_SIZE);
}

void internal_flash_program_page(uint32_t addr, const uint8_t *buf);

#endif // _INTERNAL_FLASH_H
