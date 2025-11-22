#ifndef _BL_H
#define _BL_H

#include "py32f0xx.h"
#include <assert.h>

#define BL_ADDR FLASH_BASE
#define BL_SIZE 0x2800 // 10 KB
#define BL_PAGE_NUM (BL_SIZE / FLASH_PAGE_SIZE)

static_assert(0 == BL_SIZE % FLASH_PAGE_SIZE);

#endif // _BL_H
