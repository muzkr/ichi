
#ifndef _UF2_H
#define _UF2_H

#define UF2_MAGIC_START0 0x0A324655UL // "UF2\n"
#define UF2_MAGIC_START1 0x9E5D5157UL // Randomly selected
#define UF2_MAGIC_END 0x0AB16F30UL    // Ditto

// If set, the block is "comment" and should not be flashed to the device
#define UF2_FLAG_NOFLASH 0x00000001
#define UF2_FLAG_FAMILYID_PRESENT 0x00002000

typedef struct
{
    // 32 byte header
    uint32_t magic_start0;
    uint32_t magic_start1;
    uint32_t flags;
    uint32_t target_addr;
    uint32_t payload_size;
    uint32_t block_no;
    uint32_t num_blocks;
    uint32_t family_ID;

    // raw data;
    uint8_t data[476];

    // store magic also at the end to limit damage from partial block reads
    uint32_t magic_end;
} uf2_block_t;

#endif // _UF2_H
