# ESP-IDF component wrapper for core/.
#
# It holds no source of its own. The source list is in core/sources.cmake and
# is shared with the plain CMake build.

set(CD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)

include(${CD_ROOT}/core/sources.cmake)

# WHOLE_ARCHIVE links every object file of the component. The TinyUSB
# callbacks in usb_callbacks.c are referenced only by TinyUSB. Without this
# option the linker does not extract that object from the archive. TinyUSB
# then fails to link, or uses its weak default for tud_descriptor_bos_cb and
# tud_vendor_control_xfer_cb.
idf_component_register(
    SRCS ${CD_CORE_SOURCES}
    INCLUDE_DIRS ${CD_CORE_INCLUDE}
    REQUIRES tinyusb
    WHOLE_ARCHIVE
)
