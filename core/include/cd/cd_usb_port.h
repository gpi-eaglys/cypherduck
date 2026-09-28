/* Interface to the MCU USB peripheral. */
#ifndef CD_CD_USB_PORT_H
#define CD_CD_USB_PORT_H

#include <stddef.h>
#include <stdint.h>

/* Descriptor accessors. The USB stack calls these when the host requests a
   descriptor. The returned pointers address constant data in the firmware
   image and stay valid for the lifetime of the program. */

const uint8_t *cd_device_descriptor(size_t *length);
const uint8_t *cd_configuration_descriptor(uint8_t index, size_t *length);

/* Returns the ASCII text of string descriptor `index`, or NULL if the index is
   not defined. Index 0 is the language identifier and is handled separately by
   cd_string_language(). The USB stack converts the text to UTF-16LE. */
const char *cd_string_descriptor(uint8_t index);
uint16_t    cd_string_language(void);

/* Microsoft OS 2.0 binding. The host reads the BOS descriptor, then issues a
   device-to-host request with bRequest set to cd_ms_vendor_code() and wIndex
   set to 7 to fetch the descriptor set. Windows uses both to bind WinUSB
   without an installer. Linux and macOS ignore them. */

const uint8_t *cd_bos_descriptor(size_t *length);
const uint8_t *cd_ms_os_20_descriptor(size_t *length);
uint8_t        cd_ms_vendor_code(void);

#endif /* CD_CD_USB_PORT_H */
