
#include "log.h"
#include "lwrb/lwrb.h"
#include <string.h>

#define BUF_SIZE (1024 * 2)

static uint8_t log_buf[BUF_SIZE];
static lwrb_t log_rb;

void log_init()
{
    lwrb_init(&log_rb, log_buf, sizeof(log_buf));
}

uint32_t log_fetch(uint8_t *buf, uint32_t size)
{
    lwrb_t *rb = &log_rb;
    const uint32_t avail = lwrb_get_linear_block_read_length(rb);
    if (!avail)
    {
        return 0;
    }

    if (size > avail)
    {
        size = avail;
    }
    void *a = lwrb_get_linear_block_read_address(rb);
    memcpy(buf, a, size);

    lwrb_skip(rb, size);

    return size;
}

void _putchar(char c)
{
    lwrb_write(&log_rb, &c, 1);
}
