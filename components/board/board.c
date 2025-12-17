/**
 * @file board.c
 * @brief Board HAL - Bus initialization implementation
 */

#include "board.h"
#include "esp_log.h"

static const char *TAG = "board";

// I2C bus handle
i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t board_i2c_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C (SDA:%d, SCL:%d)", BOARD_I2C_SDA, BOARD_I2C_SCL);

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = BOARD_I2C_SCL,
        .sda_io_num = BOARD_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C OK");
    return ESP_OK;
}

esp_err_t board_spi_init(void)
{
    ESP_LOGI(TAG, "Initializing SPI (CLK:%d, MOSI:%d, MISO:%d)",
             BOARD_SD_CLK, BOARD_SD_MOSI, BOARD_SD_MISO);

    spi_bus_config_t bus_config = {
        .mosi_io_num = BOARD_SD_MOSI,
        .miso_io_num = BOARD_SD_MISO,
        .sclk_io_num = BOARD_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_config_t cs_config = {
        .pin_bit_mask = (1ULL << BOARD_SD_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cs_config);
    gpio_set_level(BOARD_SD_CS, 1);

    ESP_LOGI(TAG, "SPI OK");
    return ESP_OK;
}

esp_err_t board_uart_init(void)
{
    ESP_LOGI(TAG, "Initializing UART%d (TX:%d, RX:%d, %d baud)",
             BOARD_GPS_UART_NUM, BOARD_GPS_TX, BOARD_GPS_RX, BOARD_GPS_BAUD);

    uart_config_t uart_config = {
        .baud_rate = BOARD_GPS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(BOARD_GPS_UART_NUM, 1024, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(BOARD_GPS_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(BOARD_GPS_UART_NUM, BOARD_GPS_TX, BOARD_GPS_RX,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UART OK");
    return ESP_OK;
}

esp_err_t board_init(void)
{
    ESP_LOGI(TAG, "Board initialization");

    esp_err_t ret;

    ret = board_i2c_init();
    if (ret != ESP_OK) return ret;

    ret = board_spi_init();
    if (ret != ESP_OK) return ret;

    ret = board_uart_init();
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Board init complete");
    return ESP_OK;
}
