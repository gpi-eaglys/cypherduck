/* TinyUSB configuration for the ESP32-S3 target.
 *
 * TinyUSB includes this file by name. It states which classes are compiled,
 * the endpoint 0 packet size and the buffer sizes. It is per target, so it
 * is here and not in core/.
 *
 * CFG_TUSB_MCU is supplied by the build as a compile definition. See
 * components/tinyusb/CMakeLists.txt. */

#ifndef CD_TUSB_CONFIG_H
#define CD_TUSB_CONFIG_H

#define CFG_TUSB_OS             OPT_OS_FREERTOS
#define CFG_TUSB_OS_INC_PATH    freertos/

#define CFG_TUD_ENABLED         1
#define CFG_TUD_MAX_SPEED       OPT_MODE_FULL_SPEED

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))

/* Must match bMaxPacketSize0 in core/src/descriptors.c. */
#define CFG_TUD_ENDPOINT0_SIZE  64

/* Only the class 0xFF driver is compiled. CFG_TUD_AUDIO becomes 1 when the
 * plaintext audio function is implemented. */
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_AUDIO           0
#define CFG_TUD_VIDEO           0
#define CFG_TUD_DFU             0
#define CFG_TUD_ECM_RNDIS       0
#define CFG_TUD_USBTMC          0
#define CFG_TUD_VENDOR          1

/* A frame is 344 bytes. These buffers hold a few frames. */
#define CFG_TUD_VENDOR_RX_BUFSIZE  1024
#define CFG_TUD_VENDOR_TX_BUFSIZE  1024

#endif /* CD_TUSB_CONFIG_H */
