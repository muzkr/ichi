#ifndef _LOG_H
#define _LOG_H

#if !defined(ENABLE_LOGGING)

#define log_init() \
    do             \
    {              \
    } while (0)

#define log(fmt, ...) \
    do                \
    {                 \
    } while (0)

#else // ENABLE_LOGGING

#include <stdint.h>
#include "printf.h"

void log_init();
uint32_t log_fetch(uint8_t *buf, uint32_t size);

#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif // ENABLE_LOGGING

#endif // _LOG_H
