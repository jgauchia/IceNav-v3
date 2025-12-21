/**
 * @file storage.c
 * @brief Storage implementation for SD Card and SPIFFS (ESP-IDF native)
 */

#include "storage.h"
#include "board.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <string.h>

static const char *TAG = "storage";

// State variables
static bool sdcard_mounted = false;
static bool spiffs_mounted = false;
static sdmmc_card_t *sd_card = NULL;

// ============================================================================
// SD Card Implementation
// ============================================================================

esp_err_t storage_init_sdcard(void)
{
    if (sdcard_mounted) {
        ESP_LOGW(TAG, "SD Card already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SD Card");

    // SD host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 20000;  // 20 MHz

    // SD slot configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = BOARD_SD_CS;
    slot_config.host_id = SPI2_HOST;

    // Mount configuration
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 12,
        .allocation_unit_size = 8192
    };

    // Mount filesystem
    esp_err_t ret = esp_vfs_fat_sdspi_mount(SDCARD_MOUNT_POINT, &host, &slot_config, &mount_config, &sd_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount FAT filesystem on SD Card");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD Card: %s", esp_err_to_name(ret));
        }
        sdcard_mounted = false;
        return ret;
    }

    // Log card info using ESP_LOG
    ESP_LOGI(TAG, "SD Card mounted successfully");
    ESP_LOGI(TAG, "SD Card: %s, Type: %s",
             sd_card->cid.name,
             (sd_card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC");
    ESP_LOGI(TAG, "SD Card: Speed: %d kHz, Capacity: %llu MB",
             sd_card->max_freq_khz,
             ((uint64_t)sd_card->csd.capacity * sd_card->csd.sector_size) / (1024 * 1024));

    // Create default folders
    storage_mkdir(SDCARD_WPT_FOLDER);
    storage_mkdir(SDCARD_TRK_FOLDER);

    sdcard_mounted = true;
    return ESP_OK;
}

esp_err_t storage_deinit_sdcard(void)
{
    if (!sdcard_mounted) {
        return ESP_OK;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SDCARD_MOUNT_POINT, sd_card);
    if (ret == ESP_OK) {
        sd_card = NULL;
        sdcard_mounted = false;
        ESP_LOGI(TAG, "SD Card unmounted");
    } else {
        ESP_LOGE(TAG, "Failed to unmount SD Card: %s", esp_err_to_name(ret));
    }

    return ret;
}

// ============================================================================
// SPIFFS Implementation
// ============================================================================

esp_err_t storage_init_spiffs(void)
{
    if (spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SPIFFS partition not found");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        }
        spiffs_mounted = false;
        return ret;
    }

    // Get partition info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: total=%zu, used=%zu", total, used);
    }

    spiffs_mounted = true;
    return ESP_OK;
}

esp_err_t storage_deinit_spiffs(void)
{
    if (!spiffs_mounted) {
        return ESP_OK;
    }

    esp_err_t ret = esp_vfs_spiffs_unregister(NULL);
    if (ret == ESP_OK) {
        spiffs_mounted = false;
        ESP_LOGI(TAG, "SPIFFS unmounted");
    } else {
        ESP_LOGE(TAG, "Failed to unmount SPIFFS: %s", esp_err_to_name(ret));
    }

    return ret;
}

// ============================================================================
// Combined Initialization
// ============================================================================

esp_err_t storage_init_all(void)
{
    esp_err_t ret_sd = storage_init_sdcard();
    esp_err_t ret_spiffs = storage_init_spiffs();

    // Return OK if at least one storage is available
    if (ret_sd == ESP_OK || ret_spiffs == ESP_OK) {
        return ESP_OK;
    }

    return ESP_FAIL;
}

// ============================================================================
// Status Functions
// ============================================================================

bool storage_sdcard_mounted(void)
{
    return sdcard_mounted;
}

bool storage_spiffs_mounted(void)
{
    return spiffs_mounted;
}

esp_err_t storage_get_sdcard_info(sdcard_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sdcard_mounted || !sd_card) {
        return ESP_ERR_INVALID_STATE;
    }

    // Card name
    strncpy(info->name, (const char *)sd_card->cid.name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';

    // Card type
    if (sd_card->ocr & (1 << 30)) {
        strncpy(info->card_type, "SDHC/SDXC", sizeof(info->card_type) - 1);
    } else {
        strncpy(info->card_type, "SDSC", sizeof(info->card_type) - 1);
    }
    info->card_type[sizeof(info->card_type) - 1] = '\0';

    // Sector size and capacity
    info->sector_size = sd_card->csd.sector_size;
    info->capacity = (uint64_t)sd_card->csd.capacity * sd_card->csd.sector_size;

    // Get filesystem info
    FATFS *fs;
    DWORD fre_clust;

    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
        DWORD tot_sect = (fs->n_fatent - 2) * fs->csize;
        DWORD fre_sect = fre_clust * fs->csize;

        info->total_space = (uint64_t)tot_sect * fs->ssize;
        info->free_space = (uint64_t)fre_sect * fs->ssize;
        info->used_space = info->total_space - info->free_space;
    } else {
        info->total_space = 0;
        info->free_space = 0;
        info->used_space = 0;
    }

    return ESP_OK;
}

esp_err_t storage_get_spiffs_info(spiffs_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!spiffs_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_spiffs_info(NULL, &info->total, &info->used);
    return ret;
}

void storage_print_status(void)
{
    ESP_LOGI(TAG, "=== Storage Status ===");

    if (sdcard_mounted) {
        sdcard_info_t sd_info;
        if (storage_get_sdcard_info(&sd_info) == ESP_OK) {
            char buf[32];
            ESP_LOGI(TAG, "SD Card: %s (%s)", sd_info.name, sd_info.card_type);
            ESP_LOGI(TAG, "  Capacity: %s", storage_format_size(sd_info.capacity, buf, sizeof(buf)));
            ESP_LOGI(TAG, "  Total: %s", storage_format_size(sd_info.total_space, buf, sizeof(buf)));
            ESP_LOGI(TAG, "  Used: %s", storage_format_size(sd_info.used_space, buf, sizeof(buf)));
            ESP_LOGI(TAG, "  Free: %s", storage_format_size(sd_info.free_space, buf, sizeof(buf)));
        }
    } else {
        ESP_LOGW(TAG, "SD Card: Not mounted");
    }

    if (spiffs_mounted) {
        spiffs_info_t spiffs_info;
        if (storage_get_spiffs_info(&spiffs_info) == ESP_OK) {
            char buf[32];
            ESP_LOGI(TAG, "SPIFFS:");
            ESP_LOGI(TAG, "  Total: %s", storage_format_size(spiffs_info.total, buf, sizeof(buf)));
            ESP_LOGI(TAG, "  Used: %s", storage_format_size(spiffs_info.used, buf, sizeof(buf)));
        }
    } else {
        ESP_LOGW(TAG, "SPIFFS: Not mounted");
    }
}

// ============================================================================
// Directory Listing
// ============================================================================

void storage_list_dir(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGW(TAG, "Failed to open directory: %s", path);
        return;
    }

    ESP_LOGI(TAG, "Contents of %s:", path);

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        // Get file info
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ESP_LOGI(TAG, "  [DIR]  %s", entry->d_name);
            } else {
                char size_buf[16];
                storage_format_size(st.st_size, size_buf, sizeof(size_buf));
                ESP_LOGI(TAG, "  [FILE] %s (%s)", entry->d_name, size_buf);
            }
        } else {
            ESP_LOGI(TAG, "  %s", entry->d_name);
        }
        count++;
    }

    if (count == 0) {
        ESP_LOGI(TAG, "  (empty)");
    }

    closedir(dir);
}

// ============================================================================
// File Operations
// ============================================================================

bool storage_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

size_t storage_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

bool storage_mkdir(const char *path)
{
    struct stat st;
    // Check if already exists
    if (stat(path, &st) == 0) {
        return true;  // Already exists
    }
    return mkdir(path, 0777) == 0;
}

bool storage_remove(const char *path)
{
    return remove(path) == 0;
}

bool storage_rmdir(const char *path)
{
    return rmdir(path) == 0;
}

const char *storage_format_size(uint64_t size, char *buf, size_t buf_size)
{
    static const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int order = 0;
    double formatted = (double)size;

    while (formatted >= 1024.0 && order < 4) {
        order++;
        formatted /= 1024.0;
    }

    snprintf(buf, buf_size, "%.2f %s", formatted, suffixes[order]);
    return buf;
}

// ============================================================================
// Simple File Read/Write Helpers
// ============================================================================

int storage_read_file(const char *path, void *buffer, size_t max_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, max_size, f);
    fclose(f);

    return (int)bytes_read;
}

int storage_write_file(const char *path, const void *buffer, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }

    size_t bytes_written = fwrite(buffer, 1, size, f);
    fclose(f);

    return (int)bytes_written;
}

int storage_append_file(const char *path, const void *buffer, size_t size)
{
    FILE *f = fopen(path, "ab");
    if (!f) {
        return -1;
    }

    size_t bytes_written = fwrite(buffer, 1, size, f);
    fclose(f);

    return (int)bytes_written;
}
