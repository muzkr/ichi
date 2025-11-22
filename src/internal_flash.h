#ifndef _INTERNAL_FLASH_H
#define _INTERNAL_FLASH_H

#include <stdint.h>

void internal_flash_program_page(uint32_t addr, const uint8_t *buf);

#endif // _INTERNAL_FLASH_H
