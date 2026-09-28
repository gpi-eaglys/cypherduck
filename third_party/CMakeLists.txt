# TinyUSB, as copied into this repository. See tinyusb.VERSION and
# docs/tinyusb.md.
#
# This file is the plain CMake build, used by the STM32 target. The ESP-IDF
# build registers the same sources as a component. Both read
# tinyusb_sources.cmake.
#
# The port sets three variables before this directory is added:
#
#   CD_TUSB_MCU          the CFG_TUSB_MCU value, for example OPT_MCU_ESP32S3
#   CD_TUSB_DCD_SOURCES  the device controller driver sources for that chip
#   CD_TUSB_CONFIG_DIR   the directory holding the port's tusb_config.h

foreach(var CD_TUSB_MCU CD_TUSB_DCD_SOURCES CD_TUSB_CONFIG_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is not set. It is set by port/${CD_PORT}/target.cmake")
    endif()
endforeach()

include(${CMAKE_CURRENT_LIST_DIR}/tinyusb_sources.cmake)

add_library(tinyusb STATIC ${CD_TINYUSB_SOURCES})

# CD_TUSB_CONFIG_DIR holds tusb_config.h, which TinyUSB includes by name. That
# file states which classes are compiled, the endpoint count and the buffer
# sizes. It is per target, so it is in the port.
target_include_directories(tinyusb PUBLIC ${CD_TINYUSB_INCLUDE} ${CD_TUSB_CONFIG_DIR})
target_compile_definitions(tinyusb PUBLIC CFG_TUSB_MCU=${CD_TUSB_MCU})
