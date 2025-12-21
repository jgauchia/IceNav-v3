# Main application

idf_component_register(
    SRCS "main.c"
    REQUIRES
        board
        display
        lvgl_port
        driver
        freertos
        esp_system
        esp_psram
)
