/* ESP32-S3 entry point.
 *
 * It performs the three steps that the shared core cannot: it configures the
 * USB PHY, starts TinyUSB, and creates the task that services the stack.
 * Everything else is in core/.
 *
 * Compiled with ESP-IDF v6.1. Not tested on hardware. */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_private/usb_phy.h"

#include "tusb.h"

/* The ESP32-S3 has an internal USB PHY. It is routed to the USB OTG
 * controller and set to device mode. See docs/esp32-s3.md for the pins. */
static usb_phy_handle_t phy_handle;

static void usb_phy_start(void)
{
    usb_phy_config_t config = {
        .controller = USB_PHY_CTRL_OTG,
        .target     = USB_PHY_TARGET_INT,
        .otg_mode   = USB_OTG_MODE_DEVICE,
    };

    ESP_ERROR_CHECK(usb_new_phy(&config, &phy_handle));
}

/* TinyUSB is not interrupt driven at this level. tud_task() processes the
 * events the interrupt handler queued. It returns immediately when there is
 * nothing to do, so this loop does not block. */
static void usb_device_task(void *argument)
{
    (void) argument;

    for (;;) {
        tud_task();
    }
}

void app_main(void)
{
    /* Root hub port 0 is the USB OTG controller. It runs as a full-speed
     * device. */
    const tusb_rhport_init_t device_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    usb_phy_start();
    tusb_init(0, &device_init);

    xTaskCreate(usb_device_task, "usbd", 4096, NULL, 5, NULL);
}
