#include "usb_fs.h"
#include "dfu.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "main.h"
#include "board.h"
#include "log.h"
#include "version.h"
#include "uf2.h"
#include "bl.h"
#include "internal_flash.h"
#include "py25q16.h"

#define _VOLUME_CREATE_DATE FAT_MK_DATE(2025, 11, 1)
#define _VOLUME_CREATE_TIME FAT_MK_TIME(9, 0, 0)

#define VOLUME_ID ((_VOLUME_CREATE_DATE) << 16) | (_VOLUME_CREATE_TIME)
#define VOLUME_LABEL "ICHI       "
#define BPB_MEDIA 0xf0

#define ICHI_URL "https://github.com/muzkr/ichi"

// ------------

static const fat_boot_sector_t BOOT_SECTOR_RECORD = {
    .jump_boot = {0xeb, 0, 0},
    .OEM_name = "ICHI    ",
    .sector_size = SECTOR_SIZE,
    .sectors_per_cluster = 1,
    .reserved_sectors = FAT_SECTOR - BOOT_SECTOR,
    .num_FATs = 1,
    .root_entries = ROOT_SECTOR_NUM * DIR_ENTRIES_PER_SECTOR,
    .total_sectors16 = SECTOR_NUM,
    .media = BPB_MEDIA,
    .FAT_sectors16 = FAT_SECTOR_NUM,
    .sectors_per_track = SECTOR_NUM,
    .num_heads = 1,
    .drive_num = 0x00,
    .boot_signature = FAT_BOOT_SIGNATURE_ENABLE,
    .volume_ID = VOLUME_ID,
    .volume_label = VOLUME_LABEL,
    .fs_type = "FAT16   ",
};

static const fat_dir_entry_t VOLUME_LABEL_DIR_ENTRY = {
    .name = VOLUME_LABEL,
    .attr = FAT_DIR_ATTR_VOLUME_ID,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------

static const char UF2_INFO_CONTENT[] = //
    "Version: " MOTO_VERSION "\r\n"    //
    "Support: " ICHI_URL "\r\n"        //
    ;

#define UF2_INFO_CONTENT_SIZE (sizeof(UF2_INFO_CONTENT) - 1)

static_assert(UF2_INFO_CONTENT_SIZE <= SECTOR_SIZE);

static const fat_dir_entry_t UF2_INFO_DIR_ENTRY = {
    .name = "ICHI    TXT",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(UF2_INFO_SECTOR),
    .file_size = UF2_INFO_CONTENT_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------

static const char INDEX_HTM_CONTENT[] =     //
    "<!doctype html>\n"                     //
    "<html>"                                //
    "<body>"                                //
    "<script>\n"                            //
    "location.replace(\"" ICHI_URL "\");\n" //
    "</script>"                             //
    "</body>"                               //
    "</html>\n"                             //
    ;

#define INDEX_HTM_CONTENT_SIZE (sizeof(INDEX_HTM_CONTENT) - 1)

static_assert(INDEX_HTM_CONTENT_SIZE <= SECTOR_SIZE);

static const fat_dir_entry_t INDEX_HTM_DIR_ENTRY = {
    .name = "INDEX   HTM",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(INDEX_HTM_SECTOR),
    .file_size = INDEX_HTM_CONTENT_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// CURRENT.UF2 ------

#define CURRENT_UF2_FAT_ENTRY_FIRST DATA_SECTOR_TO_FAT_ENTRY(CURRENT_UF2_SECTOR)
#define CURRENT_UF2_FAT_ENTRY_LAST (CURRENT_UF2_FAT_ENTRY_FIRST + FW_PAGE_NUM)

static const fat_dir_entry_t CURRENT_UF2_dir_entry = {
    .name = "BL      UF2",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = CURRENT_UF2_FAT_ENTRY_FIRST,
    .file_size = BL_SIZE * 2,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// DATA.UF2 ---------

static_assert(PY25Q16_PAGE_SIZE == FLASH_PAGE_SIZE);

#define DATA_UF2_FAT_ENTRY_FIRST DATA_SECTOR_TO_FAT_ENTRY(DATA_UF2_SECTOR)
#define DATA_UF2_FAT_ENTRY_LAST (DATA_UF2_FAT_ENTRY_FIRST + DATA_UF2_SECTOR_NUM)

static const fat_dir_entry_t DATA_UF2_dir_entry = {
    .name = "DATA    UF2",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_UF2_FAT_ENTRY_FIRST,
    .file_size = PY25Q16_PAGE_NUM * SECTOR_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------------

static void on_sector_read_FAT(uint32_t sector, uint8_t *buf, uint32_t entry_first, uint32_t entry_num)
{
    const uint32_t entry_last = entry_first + entry_num;

    if (sector < FAT_ENTRY_TO_SECTOR(entry_first))
    {
        return;
    }
    if (sector > FAT_ENTRY_TO_SECTOR(entry_last - 1))
    {
        return;
    }

    for (uint32_t i = 0; i < FAT_ENTRIES_PER_SECTOR; i++)
    {
        const uint32_t entry = sector * FAT_ENTRIES_PER_SECTOR + i;

        if (entry < entry_first)
        {
            continue;
        }
        if (entry >= entry_last)
        {
            break;
        }

        if (entry_last - 1 == entry)
        {
            fat_set_word(buf + FAT_ENTRY_SIZE * i, FAT16_ENTRY_EOF);
        }
        else
        {
            fat_set_word(buf + FAT_ENTRY_SIZE * i, 1 + entry);
        }
    } // for
}

// ---------------

void usb_fs_configure_done()
{
    board_backlight_on(BOARD_DEFAULT_BACKLIGHT_DELAY);
}

void usb_fs_get_cap(uint32_t *sector_num, uint16_t *sector_size)
{
    log("get_cap\n");
    *sector_num = SECTOR_NUM;
    *sector_size = SECTOR_SIZE;
}

int usb_fs_sector_read(uint32_t sector, uint8_t *buf, uint32_t size)
{
    // log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);

    if (SECTOR_SIZE != size)
    {
        log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }
    if (0 != ((uint32_t)buf) % 4)
    {
        log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }

    memset(buf, 0, SECTOR_SIZE);

    if (BOOT_SECTOR == sector)
    {
        memcpy(buf, &BOOT_SECTOR_RECORD, sizeof(BOOT_SECTOR_RECORD));
        fat_set_word(buf + SECTOR_SIZE - 2, FAT_SIGNATURE_WORD);
    }
    else if (sector < FAT_SECTOR)
    {
        // Preserved
    }
    else if (sector < ROOT_SECTOR)
    {
        // FAT ----

        sector -= FAT_SECTOR;

        if (0 == sector)
        {
            // Reserved entries
            // #0
            buf[0] = BPB_MEDIA;
            buf[1] = 0xff;
            // #1
            buf[2] = 0xff;
            buf[3] = 0xff;

            // INFO_UF2.TXT
            fat_set_word(buf + 2 * FAT_ENTRY_SIZE, FAT16_ENTRY_EOF);
            // INDEX.HTM
            fat_set_word(buf + 3 * FAT_ENTRY_SIZE, FAT16_ENTRY_EOF);
        }

        // CURRENT.UF2
        on_sector_read_FAT(sector, buf, CURRENT_UF2_FAT_ENTRY_FIRST, BL_PAGE_NUM);
        // DATA.UF2
        on_sector_read_FAT(sector, buf, DATA_UF2_FAT_ENTRY_FIRST, PY25Q16_PAGE_NUM);
    }
    else if (sector < DATA_SECTOR)
    {
        // Root ----

        sector -= ROOT_SECTOR;

        if (0 == sector)
        {
            // Volume label
            memcpy(buf + FAT_DIR_ENTRY_SIZE * VOLUME_LABEL_ROOT_ENTRY, &VOLUME_LABEL_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // INFO_UF2.TXT
            memcpy(buf + FAT_DIR_ENTRY_SIZE * UF2_INFO_ROOT_ENTRY, &UF2_INFO_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // INDEX.HTM
            memcpy(buf + FAT_DIR_ENTRY_SIZE * INDEX_HTM_ROOT_ENTRY, &INDEX_HTM_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // CURRENT.UF2
            memcpy(buf + FAT_DIR_ENTRY_SIZE * CURRENT_UF2_ROOT_ENTRY, &CURRENT_UF2_dir_entry, FAT_DIR_ENTRY_SIZE);
            // DATA.UF2
            memcpy(buf + FAT_DIR_ENTRY_SIZE * DATA_UF2_ROOT_ENTRY, &DATA_UF2_dir_entry, FAT_DIR_ENTRY_SIZE);
        }
    }
    else if (sector < SECTOR_NUM)
    {
        // Data -----

        sector -= DATA_SECTOR;

        // INFO_UF2.TXT
        if (UF2_INFO_SECTOR == sector)
        {
            memcpy(buf, UF2_INFO_CONTENT, UF2_INFO_CONTENT_SIZE);
        }
        // INDEX.HTM
        else if (INDEX_HTM_SECTOR == sector)
        {
            memcpy(buf, INDEX_HTM_CONTENT, INDEX_HTM_CONTENT_SIZE);
        }
        // CURRENT.UF2
        else if (CURRENT_UF2_SECTOR <= sector && sector < CURRENT_UF2_SECTOR + BL_PAGE_NUM)
        {
            const uint32_t target_addr = BL_ADDR + FLASH_PAGE_SIZE * (sector - CURRENT_UF2_SECTOR);
            uf2_block_t *block = (uf2_block_t *)buf;
            memcpy(block->data, (void *)target_addr, FLASH_PAGE_SIZE);

            block->magic_start0 = UF2_MAGIC_START0;
            block->magic_start1 = UF2_MAGIC_START1;
            block->target_addr = target_addr;
            block->payload_size = FLASH_PAGE_SIZE;
            block->block_no = sector - CURRENT_UF2_SECTOR;
            block->num_blocks = BL_PAGE_NUM;
            block->magic_end = UF2_MAGIC_END;
        }
        // DATA.UF2
        else if (DATA_UF2_SECTOR <= sector && sector < DATA_UF2_SECTOR + PY25Q16_PAGE_NUM)
        {
            const uint32_t data_addr = FLASH_PAGE_SIZE * (sector - DATA_UF2_SECTOR);
            uf2_block_t *block = (uf2_block_t *)buf;
            py25q16_read_page(data_addr, block->data);

            block->magic_start0 = UF2_MAGIC_START0;
            block->magic_start1 = UF2_MAGIC_START1;
            block->target_addr = data_addr;
            block->payload_size = FLASH_PAGE_SIZE;
            block->block_no = sector - CURRENT_UF2_SECTOR;
            block->num_blocks = PY25Q16_PAGE_NUM;
            block->magic_end = UF2_MAGIC_END;
        }
    }

    return 0;
}
