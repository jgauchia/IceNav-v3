/**
 * @file storage.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Storage definition and functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#include "storage.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"

extern const uint8_t SD_CS;
extern const uint8_t SD_MISO;
extern const uint8_t SD_MOSI;
extern const uint8_t SD_CLK;

static const char *TAG = "storage";

/**
 * @brief Storage Class constructor
 */
Storage::Storage() : isSdLoaded(false), card(nullptr) {}

/**
 * @brief SD Card init with DMA using ESP-IDF
 */
esp_err_t Storage::initSD()
{
    esp_err_t ret;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
#ifdef TDECK_ESP32S3
    host.slot = SPI2_HOST;
#endif
#ifdef ICENAV_BOARD
    host.slot = SPI2_HOST;
#endif
#ifdef ESP32S3_N16R8
    host.slot = SPI2_HOST;
#endif
#ifdef ESP32_N16R4
    host.slot = HSPI_HOST;
#endif

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t)SD_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = (gpio_num_t)SD_MOSI,
        .miso_io_num = (gpio_num_t)SD_MISO,
        .sclk_io_num = (gpio_num_t)SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096, // Set transfer size to 4096 bytes (multiple of 512)
        .flags = 0,
        .intr_flags = 0};

    // Adjust the SPI speed (frequency)
    host.max_freq_khz = 20000;

    // Initialize the SPI bus
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return ret;
    }

    ESP_LOGI(TAG, "Initializing SD card");

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set format_if_mount_failed = true.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return ret;
    }
    else
    {
        ESP_LOGI(TAG, "SD card initialized successfully");
        sdmmc_card_print_info(stdout, card);
        isSdLoaded = true;

        return ESP_OK;
    }
}

/**
 * @brief SPIFFS initialization
 *
 * @return esp_err_t Error code for SPIFFS setup
 */
esp_err_t Storage::initSPIFFS()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf =
        {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        else if (ret == ESP_ERR_NOT_FOUND)
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        else
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    else
        ESP_LOGI(TAG, "Partition size: total: %d used: %d", total, used);

    return ESP_OK;
}

/**
 * @brief Get SD status
 * 
 * @return true if SD card is loaded, false otherwise
 */
bool Storage::getSdLoaded() const
{
    return isSdLoaded;
}

/**
 * @brief Open a file on the SD card
 * 
 * @param path Path to the file
 * @param mode Mode in which to open the file
 * @return FILE* Pointer to the opened file
 */
FILE *Storage::open(const char *path, const char *mode)
{
    return fopen(path, mode);
}

/**
 * @brief Close a file on the SD card
 * 
 * @param file Pointer to the file
 * @return int 0 on success, EOF on error
 */
int Storage::close(FILE *file)
{
    return fclose(file);
}

/**
 * @brief Get the size of a file on the SD card
 * 
 * @param path Path to the file
 * @return size_t Size of the file in bytes
 */
size_t Storage::size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
    {
        return st.st_size;
    }
    return 0;
}

/**
 * @brief Read a specified number of bytes from a file into a buffer
 * 
 * @param file Pointer to the file
 * @param buffer Buffer to read the bytes into
 * @param size Number of bytes to read
 * @return size_t Number of bytes actually read
 */
size_t Storage::read(FILE *file, uint8_t *buffer, size_t size)
{
    if (!file)
        return 0;
    return fread(buffer, 1, size, file);
}

/**
 * @brief Read a specified number of chars from a file into a buffer
 * 
 * @param file Pointer to the file
 * @param buffer Buffer to read the chars into
 * @param size Number of bytes to read
 * @return size_t Number of chars actually read
 */
size_t Storage::read(FILE *file, char *buffer, size_t size)
{
    if (!file)
        return 0;
    return fread(buffer, 1, size, file);
}

/**
 * @brief Check if a file exists on the SD card
 * 
 * @param path Path to the file
 * @return true if the file exists, false otherwise
 */
bool Storage::exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * @brief Create a directory on the SD card
 * 
 * @param path Path to the directory
 * @return true if the directory was created successfully, false otherwise
 */
bool Storage::mkdir(const char *path)
{
    return ::mkdir(path, 0777) == 0;
}

/**
 * @brief Remove a file from the SD card
 * 
 * @param path Path to the file
 * @return true if the file was removed successfully, false otherwise
 */
bool Storage::remove(const char *path)
{
    return ::remove(path) == 0;
}

/**
 * @brief Remove a directory from the SD card
 * 
 * @param path Path to the directory
 * @return true if the directory was removed successfully, false otherwise
 */
bool Storage::rmdir(const char *path)
{
    return ::rmdir(path) == 0;
}