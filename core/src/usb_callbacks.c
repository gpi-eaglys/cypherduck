/* TinyUSB descriptor callbacks.
 *
 * TinyUSB requests descriptors through fixed callback names. This file
 * implements them by calling the accessors in descriptors.c.
 *
 * The separation keeps descriptors.c free of TinyUSB headers, so the
 * descriptor data can be compiled and checked without the stack. */

#include <string.h>

#include "tusb.h"

#include "cd/cd_usb_port.h"

uint8_t const *tud_descriptor_device_cb(void)
{
    size_t length;
    return cd_device_descriptor(&length);
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    size_t length;
    return cd_configuration_descriptor(index, &length);
}

uint8_t const *tud_descriptor_bos_cb(void)
{
    size_t length;
    return cd_bos_descriptor(&length);
}

/* A string descriptor is returned as UTF-16LE. The first unit holds the
 * descriptor type and the total length in bytes. descriptors.c stores the
 * text as ASCII, so it is converted here.
 *
 * The buffer is static because TinyUSB reads it after this function returns.
 * Control transfers are not concurrent, so one buffer is sufficient. */
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    static uint16_t buffer[32];
    size_t count;

    if (index == 0) {
        buffer[1] = cd_string_language();
        count = 1;
    } else {
        const char *text = cd_string_descriptor(index);
        if (text == NULL) {
            return NULL;
        }

        count = strlen(text);
        if (count > (sizeof buffer / sizeof buffer[0]) - 1) {
            count = (sizeof buffer / sizeof buffer[0]) - 1;
        }

        for (size_t i = 0; i < count; i++) {
            buffer[i + 1] = (uint16_t) text[i];
        }
    }

    /* Type in the high byte, total length in bytes in the low byte. */
    buffer[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | ((count + 1) * 2));

    return buffer;
}

/* Windows fetches the Microsoft OS 2.0 descriptor set with a device-to-host
 * request. bRequest is the code published in the BOS descriptor and wIndex is
 * 7. Any other request on this interface is not handled here. */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        request->bRequest == cd_ms_vendor_code() &&
        request->wIndex == 7) {
        size_t length;
        const uint8_t *descriptor = cd_ms_os_20_descriptor(&length);

        return tud_control_xfer(rhport, request,
                                (void *) (uintptr_t) descriptor,
                                (uint16_t) length);
    }

    return false;
}
