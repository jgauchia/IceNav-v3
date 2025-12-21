/**
 * @file storage.h
 * @brief Storage interface for SD Card and SPIFFS
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mount points
#define SDCARD_MOUNT_POINT      "/sdcard"
#define SPIFFS_MOUNT_POINT      "/spiffs"

// SD Card folders
#define SDCARD_WPT_FOLDER       "/sdcard/WPT"
#define SDCARD_TRK_FOLDER       "/sdcard/TRK"

/**
 * @brief SD Card information structure
 */
typedef struct {
    char name[16];              /**< Card name */
    char card_type[16];         /**< Card type (SDHC, SDXC, etc.) */
    uint32_t sector_size;       /**< Sector size in bytes */
    uint64_t capacity;          /**< Total capacity in bytes */
    uint64_t total_space;       /**< Total usable space in bytes */
    uint64_t free_space;        /**< Free space in bytes */
    uint64_t used_space;        /**< Used space in bytes */
} sdcard_info_t;

/**
 * @brief SPIFFS information structure
 */
typedef struct {
    size_t total;               /**< Total partition size in bytes */
    size_t used;                /**< Used space in bytes */
} spiffs_info_t;

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Initialize SD Card (mount FAT filesystem)
 * @note SPI bus must be initialized before calling this
 * @return ESP_OK on success
 */
esp_err_t storage_init_sdcard(void);

/**
 * @brief Unmount SD Card
 * @return ESP_OK on success
 */
esp_err_t storage_deinit_sdcard(void);

/**
 * @brief Initialize SPIFFS filesystem
 * @return ESP_OK on success
 */
esp_err_t storage_init_spiffs(void);

/**
 * @brief Unmount SPIFFS filesystem
 * @return ESP_OK on success
 */
esp_err_t storage_deinit_spiffs(void);

/**
 * @brief Initialize all storage (SD Card + SPIFFS)
 * @return ESP_OK if at least one initialized successfully
 */
esp_err_t storage_init_all(void);

// ============================================================================
// Status
// ============================================================================

/**
 * @brief Check if SD Card is mounted
 * @return true if mounted
 */
bool storage_sdcard_mounted(void);

/**
 * @brief Check if SPIFFS is mounted
 * @return true if mounted
 */
bool storage_spiffs_mounted(void);

/**
 * @brief Get SD Card information
 * @param info Pointer to store information
 * @return ESP_OK on success
 */
esp_err_t storage_get_sdcard_info(sdcard_info_t *info);

/**
 * @brief Get SPIFFS information
 * @param info Pointer to store information
 * @return ESP_OK on success
 */
esp_err_t storage_get_spiffs_info(spiffs_info_t *info);

/**
 * @brief Print storage status to console
 */
void storage_print_status(void);

/**
 * @brief List directory contents to console
 * @param path Directory path
 */
void storage_list_dir(const char *path);

// ============================================================================
// File Operations (work with both SD Card and SPIFFS based on path)
// ============================================================================

/**
 * @brief Check if file or directory exists
 * @param path Full path (e.g., "/sdcard/file.txt" or "/spiffs/data.bin")
 * @return true if exists
 */
bool storage_exists(const char *path);

/**
 * @brief Get file size
 * @param path Full path
 * @return File size in bytes, or 0 if not found
 */
size_t storage_file_size(const char *path);

/**
 * @brief Create directory
 * @param path Full path
 * @return true on success
 */
bool storage_mkdir(const char *path);

/**
 * @brief Remove file
 * @param path Full path
 * @return true on success
 */
bool storage_remove(const char *path);

/**
 * @brief Remove directory
 * @param path Full path
 * @return true on success
 */
bool storage_rmdir(const char *path);

/**
 * @brief Format human-readable size string
 * @param size Size in bytes
 * @param buf Buffer to store result
 * @param buf_size Buffer size
 * @return Pointer to buf
 */
const char *storage_format_size(uint64_t size, char *buf, size_t buf_size);

// ============================================================================
// Simple file read/write helpers
// ============================================================================

/**
 * @brief Read entire file into buffer
 * @param path Full path
 * @param buffer Buffer to store content
 * @param max_size Maximum bytes to read
 * @return Bytes read, or -1 on error
 */
int storage_read_file(const char *path, void *buffer, size_t max_size);

/**
 * @brief Write buffer to file
 * @param path Full path
 * @param buffer Data to write
 * @param size Number of bytes to write
 * @return Bytes written, or -1 on error
 */
int storage_write_file(const char *path, const void *buffer, size_t size);

/**
 * @brief Append buffer to file
 * @param path Full path
 * @param buffer Data to append
 * @param size Number of bytes to append
 * @return Bytes written, or -1 on error
 */
int storage_append_file(const char *path, const void *buffer, size_t size);

#ifdef __cplusplus
}
#endif
