#ifndef _DFU_H
#define _DFU_H

#include "fat.h"
#include "bl.h"
#include <assert.h>

#define SECTOR_SIZE FAT_DEFAULT_SECTOR_SIZE
#define SECTOR_NUM 32000

#define BOOT_SECTOR 0
#define FAT_SECTOR 1
#define FAT_SECTOR_NUM 125
#define ROOT_SECTOR (FAT_SECTOR + FAT_SECTOR_NUM)
#define ROOT_SECTOR_NUM 32
#define DATA_SECTOR (ROOT_SECTOR + ROOT_SECTOR_NUM)
#define DATA_SECTOR_NUM (SECTOR_NUM - DATA_SECTOR)

#define FAT_ENTRY_SIZE 2
#define FAT_ENTRIES_PER_SECTOR (SECTOR_SIZE / FAT_ENTRY_SIZE)

#define DIR_ENTRIES_PER_SECTOR (SECTOR_SIZE / FAT_DIR_ENTRY_SIZE)

#define DATA_SECTOR_TO_FAT_ENTRY(n) (2 + (n))
#define FAT_ENTRY_TO_SECTOR(n) ((n) / FAT_ENTRIES_PER_SECTOR)

// Root entry assign ------

#define VOLUME_LABEL_ROOT_ENTRY 0
#define UF2_INFO_ROOT_ENTRY 1
#define INDEX_HTM_ROOT_ENTRY 2
#define CURRENT_UF2_ROOT_ENTRY 3
#define DATA_UF2_ROOT_ENTRY 4

// Data sectors assign -----

#define UF2_INFO_SECTOR 0    // Data sector of UF2.TXT
#define INDEX_HTM_SECTOR 1   // First data sector of INDEX.HTM
#define CURRENT_UF2_SECTOR 2 // First data sector of CURRENT.UF2
// First data sector of DATA.UF2
#define DATA_UF2_SECTOR (CURRENT_UF2_SECTOR + BL_PAGE_NUM)

#endif // _DFU_H
