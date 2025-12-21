idf_component_register(
    SRCS
        "src/display.cpp"
    INCLUDE_DIRS
        "src"
    REQUIRES
        driver
        spi_flash
        esp_lcd
        esp_timer
        freertos
        esp_system
        esp_rom
        hal
)

# Add LovyanGFX include path (relative from components/display to project root)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${COMPONENT_DIR}/../../.pio/libdeps/ICENAV_BOARD/LovyanGFX/src"
)
