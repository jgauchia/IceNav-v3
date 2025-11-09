/**
 * @file utils.h
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version 0.2.3
 * @date 2025-11
 */

#pragma once

#ifndef DISABLE_CLI
#include "cli.hpp"
#include "storage.hpp"
#include "tft.hpp"
extern Storage storage;

/**
 * @brief Path to the temporary file used for storing screenshots.
 */
#define SCREENSHOT_TEMP_FILE "/sdcard/screenshot.png"

/**
 * @brief Captures a screenshot from the TFT display and saves it to the SD card.
 *
 * @details Creates a PNG image of the current display, attempts to write it to the specified file,
 * 			and prints the operation result to the response stream.
 *
 * @param filename Path where the screenshot will be saved.
 * @param response Stream to send operation messages.
 * @return true if the screenshot was successfully created and saved, false otherwise.
 */
static bool captureScreenshot(const char *filename, Stream *response)
{
    size_t dlen;
    uint8_t *png = (uint8_t *)tft.createPng(&dlen, 0, 0, tft.width(), tft.height());
    if (!png)
    {
        response->println("Filed to create PNG");
        return false;
    }

    FILE *file = storage.open(filename, "w");
    
    bool result = false;
    if (file)
    {
        size_t err = storage.write(file, (uint8_t *)png, dlen);
        if (err != 0)
        response->println("Screenshot saved");
        else
        response->println("Error writing screenshot");
        free(png);
        storage.close(file);
        result = true;
    }
    else
        response->println("Failed to open file for writing");

    return result;
}

/**
 * @brief WiFi client used for network operations in the CLI.
 */
static WiFiClient client;

/**
 * @brief Captures a screenshot and sends it over WiFi to a remote PC.
 *
 * @details Connects to a specified IP and port, captures a screenshot using captureScreenshot(),
 * 			then reads the file and sends its contents over the network connection.
 * 			Reports status and errors to the response stream.
 *
 * @param filename Path where the screenshot will be saved and read from.
 * @param pc_ip IP address of the remote PC to send the screenshot to.
 * @param pc_port Port of the remote PC to send the screenshot to.
 * @param response Stream to send operation messages.
 */
static void captureScreenshot(const char *filename, const char *pc_ip, uint16_t pc_port, Stream *response)
{
    if (!client.connect(pc_ip, pc_port))
    {
        response->println("Connection to server failed");
        return;
    }

    response->println("Connected to server");

    if (!captureScreenshot(filename, response))
    {
        client.stop();
        return;
    }

    FILE* file = storage.open(filename, "r");
    if (!file)
    {
        response->println("Failed to open file for reading");
        client.stop();
        return;
    }

    // Send the file data to the PC
    while (storage.fileAvailable(file))
    {
        size_t size = 0;
        uint8_t buffer[512];
        size = storage.read(file, buffer, sizeof(buffer));
        if (size > 0)
        {
        client.write(buffer, size);
        }
    }

    storage.close(file);
    client.stop();
    response->println("Screenshot sent over WiFi");
}
#endif