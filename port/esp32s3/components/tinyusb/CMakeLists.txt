# ESP-IDF component wrapper for the copied TinyUSB.
#
# It holds no source of its own. The source list is in
# third_party/tinyusb_sources.cmake and is shared with the plain CMake build.

set(CD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)

# The ESP32-S3 uses a Synopsys DWC2 core. dwc2_esp32.h in the same directory
# supplies the chip-specific register values.
set(CD_TUSB_DCD_SOURCES
    ${CD_ROOT}/third_party/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
    ${CD_ROOT}/third_party/tinyusb/src/portable/synopsys/dwc2/dwc2_common.c
)

include(${CD_ROOT}/third_party/tinyusb_sources.cmake)

idf_component_register(
    SRCS ${CD_TINYUSB_SOURCES}
    INCLUDE_DIRS ${CD_TINYUSB_INCLUDE} ${CMAKE_CURRENT_LIST_DIR}/../../main
    REQUIRES freertos soc hal esp_hw_support driver esp_rom
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC CFG_TUSB_MCU=OPT_MCU_ESP32S3)
