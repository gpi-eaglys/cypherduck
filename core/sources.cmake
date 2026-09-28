# Source list for core/. Included by core/CMakeLists.txt for the plain CMake
# build, and by port/esp32s3/components/cd_core/CMakeLists.txt for the ESP-IDF
# build. The list is written once.

set(CD_CORE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CD_CORE_SOURCES
    ${CD_CORE_DIR}/src/descriptors.c
    ${CD_CORE_DIR}/src/usb_callbacks.c
    ${CD_CORE_DIR}/src/protocol.c
    ${CD_CORE_DIR}/src/jitter.c
    ${CD_CORE_DIR}/src/crypto.c
)

set(CD_CORE_INCLUDE ${CD_CORE_DIR}/include)
