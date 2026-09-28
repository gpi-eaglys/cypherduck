# Source list for the copied TinyUSB. Included by third_party/CMakeLists.txt
# for the plain CMake build, and by
# port/esp32s3/components/tinyusb/CMakeLists.txt for the ESP-IDF build.
#
# TinyUSB supplies its own src/CMakeLists.txt. It is not used: it compiles the
# host stack and every class driver, and it adds lib/networking as an include
# directory, which is not part of this copy.
#
# CD_TUSB_DCD_SOURCES is set by the port and holds the device controller
# driver for that chip.

set(CD_TINYUSB_SRC ${CMAKE_CURRENT_LIST_DIR}/tinyusb/src)

set(CD_TINYUSB_SOURCES
    ${CD_TINYUSB_SRC}/tusb.c
    ${CD_TINYUSB_SRC}/common/tusb_fifo.c
    ${CD_TINYUSB_SRC}/device/usbd.c
    ${CD_TINYUSB_SRC}/device/usbd_control.c

    # Class 0xFF, bulk endpoints. This carries the encrypted audio frames and
    # the status replies. The audio class driver, class/audio/audio_device.c,
    # is added here when the plaintext function is implemented.
    ${CD_TINYUSB_SRC}/class/vendor/vendor_device.c

    ${CD_TUSB_DCD_SOURCES}
)

set(CD_TINYUSB_INCLUDE ${CD_TINYUSB_SRC})
