# ESP-IDF component wrapper for core/.
#
# It holds no source of its own. The source list is in core/sources.cmake and
# is shared with the plain CMake build.

set(CD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)

include(${CD_ROOT}/core/sources.cmake)

idf_component_register(
    SRCS ${CD_CORE_SOURCES}
    INCLUDE_DIRS ${CD_CORE_INCLUDE}
    REQUIRES tinyusb
)
