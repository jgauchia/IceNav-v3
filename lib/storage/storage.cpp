/**
 * @file storage.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Storage definition and functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "storage.hpp"


bool isSdLoaded = false;
extern gpio_num_t SD_CS;
extern gpio_num_t SD_MISO;
extern gpio_num_t SD_MOSI;
extern gpio_num_t SD_CLK;

/**
 * @brief SD Card init
 *
 */
esp_err_t initSD()
{
  sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  #ifdef ARDUINO_ESP32S3_DEV
  device_config.host_id = SPI2_HOST;
  #endif
  #ifdef ARDUINO_ESP32_DEV
  device_config.host_id = HSPI_HOST;
  #endif
  device_config.gpio_cs = SD_CS;  

  log_i("Initializing SD card");
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = device_config.host_id;
 // host.max_freq_khz = SDMMC_FREQ_PROBING;

  esp_vfs_fat_mount_config_t mount_config = 
  {
    //.format_if_mount_failed = true,
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  log_i("Initializing SPI BUS");
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = SD_MOSI,
    .miso_io_num = SD_MISO,
    .sclk_io_num = SD_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
  };
  esp_err_t ret = spi_bus_initialize(device_config.host_id, &bus_cfg, SDSPI_DEFAULT_DMA);

  if (ret != ESP_OK) 
  {
    log_e("Failed to initialize bus.");
    return ESP_FAIL;
  }

  digitalWrite(10,HIGH);

  log_i("Mounting filesystem");
  ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &device_config, &mount_config, &sdcard);
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL) 
    {
      log_e("Failed to mount filesystem. If you want the card to be formatted, enable above in mount_config.");
      return ESP_FAIL;
    } 
    else 
    {
      log_e("Failed to initialize the card (%s). Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      return ESP_FAIL;
    }
    return ESP_FAIL;
  }
  log_i("Filesystem mounted");

  host.set_card_clk(host.slot, 10000);

  sdmmc_card_print_info(stdout, sdcard);

  isSdLoaded = true;
  return ESP_OK;

 }

/**
 * @brief SPIFFS Init
 *
 */
esp_err_t initSPIFFS()
{
  log_i("Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = false
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret !=ESP_OK)
  {
    if (ret == ESP_FAIL)
      log_e("Failed to mount or format filesystem");
    else if (ret == ESP_ERR_NOT_FOUND)
      log_e("Failed to find SPIFFS partition");
    else
      log_e("Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret!= ESP_OK)
    log_e("Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  else
    log_i("Partition size: total: %d used: %d", total, used);
  
  return ESP_OK;
}
