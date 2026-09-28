# Cypherduck firmware.
#
# This file builds the STM32 target with plain CMake. The ESP32-S3 target is
# built by ESP-IDF, which registers core/ as a component and supplies TinyUSB
# itself; this file is not used there. See docs/tinyusb.md.
#
# Configure with the port name, for example:
#
#   cmake -B build -DCD_PORT=stm32
#
cmake_minimum_required(VERSION 3.17)

project(cypherduck C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED CD_PORT)
    message(FATAL_ERROR "CD_PORT is not set. Pass -DCD_PORT=<name>, matching a directory under port/.")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/port/${CD_PORT}/target.cmake)
    message(FATAL_ERROR "port/${CD_PORT}/target.cmake does not exist.")
endif()

# The port states which chip is built. It sets CD_TUSB_MCU,
# CD_TUSB_DCD_SOURCES and CD_TUSB_CONFIG_DIR, which third_party/ requires.
include(port/${CD_PORT}/target.cmake)

add_subdirectory(third_party)
add_subdirectory(core)
add_subdirectory(port/${CD_PORT})
