# Portable core. No HAL dependencies. Linked by every target build.
#
# It sits above TinyUSB: it supplies the descriptors and answers the class
# 0xFF endpoint callbacks. It contains no MCU-specific code.
#
# This file is the plain CMake build, used by the STM32 target. The ESP-IDF
# build registers the same sources as a component. Both read sources.cmake.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

add_library(cd_core STATIC ${CD_CORE_SOURCES})
target_include_directories(cd_core PUBLIC ${CD_CORE_INCLUDE})
target_link_libraries(cd_core PUBLIC tinyusb)
