# Ichi: Bootloader Updater for Quansheng UV-K5 (V3) and Variants

Ichi ("いち/一", Japanese number 1 :one:) is a bootloader update program for the QS UV-K5 (V3) and variants (UV-K6, UV-K1, etc).

Ichi mounts the radio as a USB disk on the computer, offering a drag-and-drop experience that frees users from the hassle of special tools or command lines.

Features: 

- Mounts the radio as a USB disk to provide a drag-and-drop user experience
- Bootloader readout and update
- Data storage (SPI flash) read and write
- FAT file system
- UF2 file format


## Usage

> [!CAUTION]
> Ichi is easy-to-use yet very powerful! Careless or improper handling can lead to catastrophic consequences, such as data corruption including calibration data. No one other than the user themselves — including the author of Ichi — shall be held responsible for any consequences.

> [!WARNING]
> Updating the bootloader is a risky operation. An incomplete update may render the device unable to boot again. 

Install Ichi as regulat firmware onto the radio. After powering on, connect the USB-C port to a computer. The radio will mount as a USB disk on the computer (labeled "ICHI"). 

Inside the ICHI disk, you will find:

- BL.UF2: Current bootloader
- DATA.UF2: The complete data storage (SPI flash)

You can copy these files to a safe place as backups.

Copy the bootloader program (in UF2 format) that you wish to update to the radio onto the ICHI disk, and Ichi will flash it to the internal flash memory, replacing the original bootloader.

You can also update SPI flash data. Simply copy the data file in UF2 format to the ICHI disk, and Ichi will do the rest. 

For detailed instructions, please refer to the [wiki](https://github.com/muzkr/ichi/wiki).

> [!IMPORTANT]
> Please be sure to carefully read the wiki content to gain an understanding of the underlying mechanisms, common operation instructions, and caveats about pitfalls to avoid.

For your convenience, some of the stock bootloaders can be found in the `archive` folder of this repository.

## Licensing

Apache 2.0

Third-party components and copyright information can be found in the 3rd_party_licenses.txt file.
