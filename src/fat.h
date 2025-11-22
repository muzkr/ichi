#ifndef _FAT_H
#define _FAT_H

#include <stdint.h>
#include <assert.h>

// Boot ----------

#define FAT_DEFAULT_SECTOR_SIZE 512
#define FAT_SIGNATURE_WORD 0xaa55
#define FAT_BOOT_SIGNATURE_ENABLE 0x29

typedef struct
{
    uint8_t jump_boot[3];
    uint8_t OEM_name[8];
    uint16_t sector_size;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_FATs;
    uint16_t root_entries;
    uint16_t total_sectors16;
    uint8_t media;
    uint16_t FAT_sectors16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors32;
    uint8_t drive_num;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_ID;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} __attribute__((packed)) fat_boot_sector_t;

static_assert(sizeof(fat_boot_sector_t) == 62);

// FAT ------------

#define FAT16_ENTRY_FREE 0
#define FAT16_ENTRY_EOF 0xffff

#define FAT12_ENTRY_FREE 0
#define FAT12_ENTRY_EOF 0xfff

// Root -------------

#define FAT_DIR_ENTRY_SIZE 32

#define FAT_MK_DATE(y, m, d) ((uint16_t)(((0x7f & ((y) - 1980)) << 9) | ((0xf & (m)) << 5) | (0x1f & (d))))
#define FAT_MK_TIME(h, m, s) ((uint16_t)(((0x1f & (h)) << 11) | ((0x3f & (m)) << 5) | (0x1f & ((s) / 2))))

typedef struct
{
    uint8_t name[11];
    uint8_t attr;
    uint8_t NTRes;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_clusterHI;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_clusterLO;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry_t;

static_assert(sizeof(fat_dir_entry_t) == FAT_DIR_ENTRY_SIZE);

enum
{
    FAT_DIR_ATTR_RO = 0x1,
    FAT_DIR_ATTR_HIDDEN = 0x2,
    FAT_DIR_ATTR_SYSTEM = 0x4,
    FAT_DIR_ATTR_VOLUME_ID = 0x8,
    FAT_DIR_ATTR_DIR = 0x10,
    FAT_DIR_ATTR_ARCHIVE = 0x20,
};

// Misc --------------

static inline void fat_set_word(uint8_t *buf, uint16_t value)
{
    buf[0] = 0xff & value;
    buf[1] = 0xff & (value >> 8);
}

static inline void fat_set_dword(uint8_t *buf, uint32_t value)
{
    for (int i = 0; i < 4; i++)
    {
        buf[i] = 0xff & value;
        value >>= 8;
    }
}

#endif
