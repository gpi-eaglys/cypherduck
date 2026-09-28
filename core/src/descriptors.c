/* Device, configuration, interface and endpoint descriptors.

   The descriptors are constant data. The host requests them during
   enumeration and the firmware returns them unchanged.

   Field names and field widths are defined by the USB 2.0 specification.
   The prefix states the width: `b` is one byte, `w` is two bytes little
   endian, `bcd` is binary coded decimal, `i` is a string descriptor index.

   This file declares one interface, class 0xFF, with two bulk endpoints.
   The audio class function is not declared here. Adding it later appends
   two interfaces, numbered 1 and 2, and does not change interface 0. */

#include "cd/cd_usb_port.h"

/* ---------------------------------------------------------------- identity */

/* pid.codes allocates product identifiers under 0x1209 for open source
   projects. A product requires an identifier from USB-IF. See section 7 of
   the design plan. */
#define CD_ID_VENDOR   0x1209
#define CD_ID_PRODUCT  0x0001
#define CD_BCD_DEVICE  0x0100  /* device version 1.00 */

/* String descriptor indices. Index 0 is the language identifier. */
#define CD_STR_MANUFACTURER  1
#define CD_STR_PRODUCT       2
#define CD_STR_SERIAL        3
#define CD_STR_INTERFACE     4

/* ------------------------------------------------------------- descriptors */

#define CD_DESC_DEVICE         0x01
#define CD_DESC_CONFIGURATION  0x02
#define CD_DESC_INTERFACE      0x04
#define CD_DESC_ENDPOINT       0x05

#define CD_EP_FRAMES  0x01  /* bulk OUT, encrypted audio frames */
#define CD_EP_STATUS  0x81  /* bulk IN, status replies          */

#define CD_EP_BULK        0x02
#define CD_EP_PACKET_MAX  64

/* Two bytes, little endian. */
#define CD_U16(v)  (uint8_t)((v) & 0xFF), (uint8_t)(((v) >> 8) & 0xFF)

static const uint8_t device_descriptor[] = {
    18,                        /* bLength            18 bytes                */
    CD_DESC_DEVICE,            /* bDescriptorType    device                  */
    CD_U16(0x0201),            /* bcdUSB             2.01, declares a BOS   */
                               /*                    descriptor             */
    0x00,                      /* bDeviceClass       defined per interface   */
    0x00,                      /* bDeviceSubClass                            */
    0x00,                      /* bDeviceProtocol                            */
    CD_EP_PACKET_MAX,          /* bMaxPacketSize0    endpoint 0 packet size  */
    CD_U16(CD_ID_VENDOR),      /* idVendor                                   */
    CD_U16(CD_ID_PRODUCT),     /* idProduct                                  */
    CD_U16(CD_BCD_DEVICE),     /* bcdDevice                                  */
    CD_STR_MANUFACTURER,       /* iManufacturer                              */
    CD_STR_PRODUCT,            /* iProduct                                   */
    CD_STR_SERIAL,             /* iSerialNumber                              */
    1,                         /* bNumConfigurations                         */
};

/* 9 for the configuration, 9 for the interface, 7 for each endpoint. */
#define CD_CONFIG_TOTAL_LENGTH  (9 + 9 + 7 + 7)

static const uint8_t configuration_descriptor[] = {
    /* Configuration */
    9,                              /* bLength                               */
    CD_DESC_CONFIGURATION,          /* bDescriptorType                       */
    CD_U16(CD_CONFIG_TOTAL_LENGTH), /* wTotalLength   every byte that follows*/
    1,                              /* bNumInterfaces                        */
    1,                              /* bConfigurationValue                   */
    0,                              /* iConfiguration no string              */
    0x80,                           /* bmAttributes   bus powered            */
    50,                             /* bMaxPower      50 x 2 mA = 100 mA     */

    /* Interface 0, class 0xFF */
    9,                              /* bLength                               */
    CD_DESC_INTERFACE,              /* bDescriptorType                       */
    0,                              /* bInterfaceNumber                      */
    0,                              /* bAlternateSetting                     */
    2,                              /* bNumEndpoints                         */
    0xFF,                           /* bInterfaceClass    device defined     */
    0x00,                           /* bInterfaceSubClass                    */
    0x00,                           /* bInterfaceProtocol                    */
    CD_STR_INTERFACE,               /* iInterface                            */

    /* Endpoint 0x01, bulk OUT, encrypted audio frames */
    7,                              /* bLength                               */
    CD_DESC_ENDPOINT,               /* bDescriptorType                       */
    CD_EP_FRAMES,                   /* bEndpointAddress                      */
    CD_EP_BULK,                     /* bmAttributes                          */
    CD_U16(CD_EP_PACKET_MAX),       /* wMaxPacketSize                        */
    0,                              /* bInterval      ignored for bulk       */

    /* Endpoint 0x81, bulk IN, status replies */
    7,                              /* bLength                               */
    CD_DESC_ENDPOINT,               /* bDescriptorType                       */
    CD_EP_STATUS,                   /* bEndpointAddress                      */
    CD_EP_BULK,                     /* bmAttributes                          */
    CD_U16(CD_EP_PACKET_MAX),       /* wMaxPacketSize                        */
    0,                              /* bInterval      ignored for bulk       */
};

/* ------------------------------------------------------------------ strings */

/* The serial number is a placeholder. Provisioning writes the device ID, and
   the port layer returns it in place of this value. */
static const char *const strings[] = {
    [CD_STR_MANUFACTURER] = "Cypherduck",
    [CD_STR_PRODUCT]      = "Cypherduck Audio Dongle",
    [CD_STR_SERIAL]       = "000000000000",
    [CD_STR_INTERFACE]    = "Cypherduck encrypted audio",
};

#define CD_STRING_COUNT  (sizeof strings / sizeof strings[0])

/* ---------------------------------------------------------------- accessors */

const uint8_t *cd_device_descriptor(size_t *length)
{
    *length = sizeof device_descriptor;
    return device_descriptor;
}

const uint8_t *cd_configuration_descriptor(uint8_t index, size_t *length)
{
    if (index != 0) {
        *length = 0;
        return NULL;
    }
    *length = sizeof configuration_descriptor;
    return configuration_descriptor;
}

const char *cd_string_descriptor(uint8_t index)
{
    if (index == 0 || index >= CD_STRING_COUNT) {
        return NULL;
    }
    return strings[index];
}

uint16_t cd_string_language(void)
{
    return 0x0409;  /* English, United States */
}

/* ----------------------------------------------- Microsoft OS 2.0 binding */

/* Windows 8.1 and later bind WinUSB to an interface without an INF file and
   without an installer when the device supplies a Microsoft OS 2.0 descriptor
   set. The set states two things: the compatible identifier WINUSB, and a
   device interface GUID that the host application opens.

   The host reaches the set in two steps:

     1. It reads the BOS descriptor, which holds a platform capability
        descriptor identified by the Microsoft OS 2.0 platform GUID. That
        descriptor states the length of the set and the request code.
     2. It issues a device-to-host request with bRequest set to that code and
        wIndex set to 7, and receives the set.

   Linux and macOS ignore all of this and use libusb. */

/* Request code for step 2. It must not collide with the control requests in
   section 5 of the design plan, which occupy 0x01 to 0x13. */
#define CD_MS_VENDOR_CODE  0x20

/* The application opens the device by this GUID. It is part of the interface
   between the firmware and the host library and does not change once units
   are in the field. */
#define CD_DEVICE_INTERFACE_GUID  "{1D4B2365-4749-48EA-B38A-7C6FDDDD7E26}"

/* One UTF-16LE code unit. The registry property fields are UTF-16LE, unlike
   the string descriptors, which the stack converts at runtime. */
#define CD_U16LE_CHAR(c)  (uint8_t)(c), 0x00

#define CD_MS_OS_20_LENGTH   178
#define CD_BOS_LENGTH        33

static const uint8_t bos_descriptor[] = {
    /* BOS */
    5,                          /* bLength                                   */
    0x0F,                       /* bDescriptorType    BOS                    */
    CD_U16(CD_BOS_LENGTH),      /* wTotalLength       including capabilities */
    1,                          /* bNumDeviceCaps                            */

    /* Platform capability, Microsoft OS 2.0 */
    28,                         /* bLength                                   */
    0x10,                       /* bDescriptorType    device capability      */
    0x05,                       /* bDevCapabilityType platform               */
    0x00,                       /* bReserved                                 */
    /* Platform GUID {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F}, mixed endian as
       the specification defines it. */
    0xDF, 0x60, 0xDD, 0xD8,
    0x89, 0x45,
    0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,     /* dwWindowsVersion   0x06030000, Windows 8.1*/
    CD_U16(CD_MS_OS_20_LENGTH), /* wMSOSDescriptorSetTotalLength             */
    CD_MS_VENDOR_CODE,          /* bMS_VendorCode                            */
    0x00,                       /* bAltEnumCode       no alternate enumeration*/
};

static const uint8_t ms_os_20_descriptor[] = {
    /* Set header, 10 bytes */
    CD_U16(10),                 /* wLength                                   */
    CD_U16(0x0000),             /* wDescriptorType    set header             */
    0x00, 0x00, 0x03, 0x06,     /* dwWindowsVersion   0x06030000             */
    CD_U16(CD_MS_OS_20_LENGTH), /* wTotalLength       the whole set          */

    /* Configuration subset, 8 bytes of header, 168 bytes with its children */
    CD_U16(8),                  /* wLength                                   */
    CD_U16(0x0001),             /* wDescriptorType    configuration subset   */
    0x00,                       /* bConfigurationValue  index, zero based    */
    0x00,                       /* bReserved                                 */
    CD_U16(168),                /* wTotalLength                              */

    /* Function subset, 8 bytes of header, 160 bytes with its children */
    CD_U16(8),                  /* wLength                                   */
    CD_U16(0x0002),             /* wDescriptorType    function subset        */
    0x00,                       /* bFirstInterface    interface 0            */
    0x00,                       /* bReserved                                 */
    CD_U16(160),                /* wSubsetLength                             */

    /* Compatible identifier, 20 bytes */
    CD_U16(20),                 /* wLength                                   */
    CD_U16(0x0003),             /* wDescriptorType    compatible ID          */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,  /* CompatibleID, 8 bytes      */
    0, 0, 0, 0, 0, 0, 0, 0,                    /* SubCompatibleID, 8 bytes   */

    /* Registry property, 132 bytes */
    CD_U16(132),                /* wLength                                   */
    CD_U16(0x0004),             /* wDescriptorType    registry property      */
    CD_U16(7),                  /* wPropertyDataType  REG_MULTI_SZ           */
    CD_U16(42),                 /* wPropertyNameLength  21 units             */
    CD_U16LE_CHAR('D'), CD_U16LE_CHAR('e'), CD_U16LE_CHAR('v'),
    CD_U16LE_CHAR('i'), CD_U16LE_CHAR('c'), CD_U16LE_CHAR('e'),
    CD_U16LE_CHAR('I'), CD_U16LE_CHAR('n'), CD_U16LE_CHAR('t'),
    CD_U16LE_CHAR('e'), CD_U16LE_CHAR('r'), CD_U16LE_CHAR('f'),
    CD_U16LE_CHAR('a'), CD_U16LE_CHAR('c'), CD_U16LE_CHAR('e'),
    CD_U16LE_CHAR('G'), CD_U16LE_CHAR('U'), CD_U16LE_CHAR('I'),
    CD_U16LE_CHAR('D'), CD_U16LE_CHAR('s'), CD_U16LE_CHAR('\0'),
    CD_U16(80),                 /* wPropertyDataLength  40 units             */
    CD_U16LE_CHAR('{'),
    CD_U16LE_CHAR('1'), CD_U16LE_CHAR('D'), CD_U16LE_CHAR('4'),
    CD_U16LE_CHAR('B'), CD_U16LE_CHAR('2'), CD_U16LE_CHAR('3'),
    CD_U16LE_CHAR('6'), CD_U16LE_CHAR('5'), CD_U16LE_CHAR('-'),
    CD_U16LE_CHAR('4'), CD_U16LE_CHAR('7'), CD_U16LE_CHAR('4'),
    CD_U16LE_CHAR('9'), CD_U16LE_CHAR('-'),
    CD_U16LE_CHAR('4'), CD_U16LE_CHAR('8'), CD_U16LE_CHAR('E'),
    CD_U16LE_CHAR('A'), CD_U16LE_CHAR('-'),
    CD_U16LE_CHAR('B'), CD_U16LE_CHAR('3'), CD_U16LE_CHAR('8'),
    CD_U16LE_CHAR('A'), CD_U16LE_CHAR('-'),
    CD_U16LE_CHAR('7'), CD_U16LE_CHAR('C'), CD_U16LE_CHAR('6'),
    CD_U16LE_CHAR('F'), CD_U16LE_CHAR('D'), CD_U16LE_CHAR('D'),
    CD_U16LE_CHAR('D'), CD_U16LE_CHAR('D'), CD_U16LE_CHAR('7'),
    CD_U16LE_CHAR('E'), CD_U16LE_CHAR('2'), CD_U16LE_CHAR('6'),
    CD_U16LE_CHAR('}'),
    CD_U16LE_CHAR('\0'),        /* terminates the string                     */
    CD_U16LE_CHAR('\0'),        /* terminates the REG_MULTI_SZ list          */
};

/* The lengths are written into the descriptors above and are also the values
   the host uses to size its requests. A mismatch produces a device that
   enumerates and then fails to bind, which is difficult to diagnose on the
   host. These checks fail the build instead. */
_Static_assert(sizeof bos_descriptor == CD_BOS_LENGTH,
               "BOS wTotalLength does not match the descriptor");
_Static_assert(sizeof ms_os_20_descriptor == CD_MS_OS_20_LENGTH,
               "MS OS 2.0 wTotalLength does not match the descriptor set");
_Static_assert(sizeof configuration_descriptor == CD_CONFIG_TOTAL_LENGTH,
               "Configuration wTotalLength does not match the descriptor");
_Static_assert(sizeof device_descriptor == 18,
               "Device descriptor bLength does not match the descriptor");

const uint8_t *cd_bos_descriptor(size_t *length)
{
    *length = sizeof bos_descriptor;
    return bos_descriptor;
}

const uint8_t *cd_ms_os_20_descriptor(size_t *length)
{
    *length = sizeof ms_os_20_descriptor;
    return ms_os_20_descriptor;
}

uint8_t cd_ms_vendor_code(void)
{
    return CD_MS_VENDOR_CODE;
}
