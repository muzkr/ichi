#include "usbd_core.h"
#include "usbd_msc.h"
#include "usb_fs.h"

#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

#define USBD_VID           0x36b7
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + MSC_DESCRIPTOR_LEN)

const uint8_t msc_flash_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, 0x02),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x0C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'M', 0x00,                  /* wcChar0 */
    'U', 0x00,                  /* wcChar1 */
    'Z', 0x00,                  /* wcChar2 */
    'K', 0x00,                  /* wcChar3 */
    'R', 0x00,                  /* wcChar4 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x0A,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'I', 0x00,                  /* wcChar0 */
    'C', 0x00,                  /* wcChar1 */
    'H', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x0A,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '0', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '4', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

void usbd_configure_done_callback(void)
{
    usb_fs_configure_done();
}

void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    usb_fs_get_cap(block_num, block_size);
}

int usbd_msc_sector_read(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    return usb_fs_sector_read(sector, buffer, length);
}

int usbd_msc_sector_write(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    return usb_fs_sector_write(sector, buffer, length);
}

struct usbd_interface intf0;

void msc_ram_init(void)
{
    usbd_desc_register(msc_flash_descriptor);
    usbd_add_interface(usbd_msc_init_intf(&intf0, MSC_OUT_EP, MSC_IN_EP));

    usbd_initialize();
}
