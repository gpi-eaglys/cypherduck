# ESP-IDF project root for the ESP32-S3 target.
#
# The ESP-IDF project lives here rather than at the repository root, because
# the root CMakeLists.txt is the plain CMake build used by the STM32 target.
# The STM32 target does not use ESP-IDF.
#
# Build:
#   idf.py -C port/esp32s3 set-target esp32s3
#   idf.py -C port/esp32s3 build
#
# components/ holds one wrapper per shared directory. Each wrapper registers
# the sources from core/ and third_party/ with ESP-IDF. The shared trees hold
# no ESP-IDF specific files, so a TinyUSB upgrade does not touch them.

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(cypherduck C)
