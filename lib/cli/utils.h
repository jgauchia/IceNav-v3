/**
 * @file utils.h
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#ifndef DISABLE_CLI
#include "cli.hpp"
#include "storage.hpp"
#include "tft.hpp"

#define SCREENSHOT_TEMP_FILE "/sdcard/screenshot.png" /**< Path to the temporary file used for storing screenshots. */

bool captureScreenshot(const char *filename, Stream *response);
void captureScreenshot(const char *filename, const char *pc_ip, uint16_t pc_port, Stream *response);
#endif