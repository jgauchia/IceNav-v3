idf_component_register(
    SRCS
        "src/lvgl_port.cpp"
    INCLUDE_DIRS
        "src"
    REQUIRES
        display
        esp_timer
        freertos
        esp_psram
)

# Add LVGL include path
target_include_directories(${COMPONENT_LIB} PUBLIC
    "${COMPONENT_DIR}/../../.pio/libdeps/ICENAV_BOARD/lvgl"
    "${COMPONENT_DIR}/../../.pio/libdeps/ICENAV_BOARD/lvgl/src"
)
