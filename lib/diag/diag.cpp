/**
 * @file diag.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boot diagnostics: reset reason logging and crash coredump recovery
 * @version 0.3.0
 * @date 2026-07
 */

#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_core_dump.h"
#include "esp_partition.h"
#include "storage.hpp"
#include "diag.hpp"

static const char *TAG = "diag";

static const char *DIAG_LOG_PATH  = "/sdcard/DIAG.log";
static const char *COREDUMP_PATH  = "/sdcard/COREDUMP.elf";
static constexpr size_t DIAG_LOG_MAX_SIZE = 64 * 1024;

/**
 * @brief Return a human-readable string for a reset reason
 *
 * @param reason Reset reason from esp_reset_reason()
 * @return Description string
 */
static const char *resetReasonStr(esp_reset_reason_t reason)
{
    switch (reason)
    {
        case ESP_RST_POWERON:   return "Power-on";
        case ESP_RST_EXT:       return "External pin";
        case ESP_RST_SW:        return "Software restart";
        case ESP_RST_PANIC:     return "Panic/exception";
        case ESP_RST_INT_WDT:   return "Interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "Task watchdog";
        case ESP_RST_WDT:       return "Other watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep wakeup";
        case ESP_RST_BROWNOUT:  return "Brownout";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "Unknown";
    }
}

/**
 * @brief Copy the coredump flash partition contents to the SD card as an ELF file
 *
 * @return true if the full image was copied
 */
static bool copyCoredumpToSD()
{
    size_t addr = 0;
    size_t size = 0;
    if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0)
        return false;

    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                           ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
    if (!part)
        return false;

    if (size > part->size || (addr - part->address) > part->size)
    {
        ESP_LOGE(TAG, "Coredump size looks corrupt (size=%u part_size=%u), skipping copy", (unsigned)size, (unsigned)part->size);
        return false;
    }

    FILE *out = storage.open(COREDUMP_PATH, "wb");
    if (!out)
        return false;

    static uint8_t chunk[1024];
    size_t offset = addr - part->address;
    size_t remaining = size;
    bool ok = true;
    while (remaining > 0)
    {
        size_t len = remaining > sizeof(chunk) ? sizeof(chunk) : remaining;
        if (esp_partition_read(part, offset, chunk, len) != ESP_OK)
        {
            ok = false;
            break;
        }
        if (storage.write(out, chunk, len) != len)
        {
            ok = false;
            break;
        }
        offset    += len;
        remaining -= len;
    }
    storage.close(out);
    if (!ok)
        storage.remove(COREDUMP_PATH);
    return ok;
}

/**
 * @brief Append crash summary details from the stored coredump to the diagnostics log
 *
 * @param log Open log file
 */
static void logCoredumpSummary(FILE *log)
{
    esp_core_dump_summary_t summary;
    if (esp_core_dump_get_summary(&summary) != ESP_OK)
    {
        storage.println(log, "Coredump found but summary unavailable");
        return;
    }

    char line[128];
    snprintf(line, sizeof(line), "Crash task: %s  PC: 0x%08lx", summary.exc_task,
             (unsigned long)summary.exc_pc);
    storage.println(log, line);

    #if __XTENSA__
        // Xtensa (ESP32-S3): cause/vaddr plus a resolved on-device backtrace.
        snprintf(line, sizeof(line), "Exception cause: %lu  vaddr: 0x%08lx",
                 (unsigned long)summary.ex_info.exc_cause, (unsigned long)summary.ex_info.exc_vaddr);
        storage.println(log, line);

        if (summary.exc_bt_info.depth > 0)
        {
            storage.print(log, "Backtrace:");
            for (uint32_t i = 0; i < summary.exc_bt_info.depth && i < 16; i++)
            {
                snprintf(line, sizeof(line), " 0x%08lx", (unsigned long)summary.exc_bt_info.bt[i]);
                storage.print(log, line);
            }
            if (summary.exc_bt_info.corrupted)
                storage.print(log, " (corrupted)");
            storage.println(log, "");
        }
    #else
        // RISC-V (ESP32-P4): mcause/mtval; the backtrace is a raw stack dump that
        // must be resolved off-device with GDB or the ELF, so it is not printed here.
        snprintf(line, sizeof(line), "Exception mcause: %lu  mtval: 0x%08lx",
                 (unsigned long)summary.ex_info.mcause, (unsigned long)summary.ex_info.mtval);
        storage.println(log, line);
    #endif
}

/**
 * @brief Log the boot reset reason and recover any stored crash coredump to the SD card
 *
 * @details Appends the reset reason to the diagnostics log on the SD card. If the coredump
 *          partition holds a crash image, its summary (task, PC, cause, backtrace) is logged,
 *          the full ELF image is copied to the SD card and the partition is erased.
 */
void diagBootReport()
{
    esp_reset_reason_t reason = esp_reset_reason();
    bool hasCoredump = (esp_core_dump_image_check() == ESP_OK);

    ESP_LOGI(TAG, "Reset reason: %s%s", resetReasonStr(reason),
             hasCoredump ? " (coredump stored)" : "");

    if (!storage.getSdLoaded())
        return;

    if (storage.exists(DIAG_LOG_PATH) && storage.size(DIAG_LOG_PATH) > DIAG_LOG_MAX_SIZE)
        storage.remove(DIAG_LOG_PATH);

    FILE *log = storage.open(DIAG_LOG_PATH, "a");
    if (!log)
    {
        ESP_LOGE(TAG, "Cannot open %s", DIAG_LOG_PATH);
        return;
    }

    char line[96];
    snprintf(line, sizeof(line), "--- Boot v%s rev %d ---", VERSION, (int)REVISION);
    storage.println(log, line);
    snprintf(line, sizeof(line), "Reset reason: %s (%d)", resetReasonStr(reason), (int)reason);
    storage.println(log, line);

    if (hasCoredump)
    {
        logCoredumpSummary(log);
        if (copyCoredumpToSD())
        {
            snprintf(line, sizeof(line), "Coredump saved to %s", COREDUMP_PATH);
            storage.println(log, line);
        }
        else
            storage.println(log, "Coredump copy to SD failed");
        esp_core_dump_image_erase();
    }

    storage.close(log);
}
