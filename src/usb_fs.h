
#ifndef _USB_FS_H
#define _USB_FS_H

#include <stdint.h>

void usb_fs_configure_done();
void usb_fs_get_cap(uint32_t *sector_num, uint16_t *sector_size);
int usb_fs_sector_read(uint32_t sector, uint8_t *buf, uint32_t size);
int usb_fs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size);

#endif