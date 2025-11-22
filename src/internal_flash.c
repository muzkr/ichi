#include "internal_flash.h"
#include <stdbool.h>
#include <string.h>
#include "py32f071_ll_flash.h"
#include "py32f071_ll_utils.h"

static inline void wait_BSY()
{
    while (LL_FLASH_IsActiveFlag_BUSY(FLASH))
    {
    }
}

static inline void wait_EOP()
{
    while (!LL_FLASH_IsActiveFlag_EOP(FLASH))
    {
    }
    LL_FLASH_ClearFlag_EOP(FLASH);
}

static bool page_need_erase(uint32_t addr)
{
    const uint8_t *p = (uint8_t *)addr;
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++)
    {
        if (0xff != p[i])
        {
            return true;
        }
    }
    return false;
}

static void page_erase(uint32_t addr)
{
    wait_BSY();
    LL_FLASH_Unlock(FLASH);
    LL_FLASH_EnablePageErase(FLASH);
    LL_FLASH_EnableIT_EOP(FLASH);
    *((uint32_t *)((addr / 4) * 4)) = 0xffffffffU;
    wait_BSY();
    wait_EOP();
    LL_FLASH_DisableIT_EOP(FLASH);
    LL_FLASH_Lock(FLASH);
}

void internal_flash_program_page(uint32_t addr, const uint8_t *buf)
{
    // Test code
    // LL_mDelay(20);
    // return;

    if (0 == memcmp((void *)addr, buf, FLASH_PAGE_SIZE))
    {
        return;
    }

    if (page_need_erase(addr))
    {
        page_erase(addr);
    }

    wait_BSY();
    LL_FLASH_Unlock(FLASH);
    LL_FLASH_EnablePageProgram(FLASH);
    LL_FLASH_EnableIT_EOP(FLASH);

    uint32_t *const p = (uint32_t *)addr;
    for (uint32_t i = 0; i < 63; i++)
    {
        uint32_t n = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
        p[i] = n;
        buf += 4;
    }

    LL_FLASH_EnablePageProgramStart(FLASH);

    do
    {
        uint32_t n = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
        p[63] = n;
    } while (0);

    wait_BSY();
    wait_EOP();
    LL_FLASH_DisableIT_EOP(FLASH);
    LL_FLASH_DisablePageProgram(FLASH);
    LL_FLASH_Lock(FLASH);
}
