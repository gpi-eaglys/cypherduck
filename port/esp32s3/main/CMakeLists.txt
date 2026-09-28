idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES cd_core tinyusb esp_hw_support driver
)
