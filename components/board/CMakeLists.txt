idf_component_register(
    SRCS "board.c"
    INCLUDE_DIRS "include"
    REQUIRES driver hal esp_driver_gpio esp_driver_i2c esp_driver_spi esp_driver_uart
)
