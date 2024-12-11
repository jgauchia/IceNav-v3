/**
 * @file utils.h
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef UTILS_H
#define UTILS_H

#ifndef DISABLE_CLI
#include "cli.hpp"
#include "storage.hpp"
#include "tft.hpp"

#define SCREENSHOT_TEMP_FILE "/screenshot.png"

// Capture the screenshot and save it to the SD card
static bool captureScreenshot(const char* filename, Stream *response)
{
  size_t dlen;
  uint8_t* png = (uint8_t*)tft.createPng(&dlen, 0, 0, tft.width(), tft.height());
  if (!png)
  {
    response->println("Filed to create PNG");
    return false;
  }

  File file = SD.open(filename, FILE_WRITE);

  bool result = false;
  if (file)
  {
    file.write((uint8_t*)png, dlen);
    response->println("Screenshot saved");
    free(png);
    file.close();
    result = true;
  }
  else  
    response->println("Failed to open file for writing");

  return result;
}

// WiFi client
static WiFiClient client;

static void captureScreenshot(const char* filename, const char* pc_ip, uint16_t pc_port, Stream *response) {
  if (!client.connect(pc_ip, pc_port)) {
    response->println("Connection to server failed");
    return;
  }

  response->println("Connected to server");
  
  if (!captureScreenshot(filename,response)){
    client.stop();
    return;
  };

  File file = SD.open(filename, FILE_READ);
  if (!file) {
    response->println("Failed to open file for reading");
    client.stop();
    return;
  }

  // Send the file data to the PC
  while (file.available()) {
    size_t size = 0;
    uint8_t buffer[512];
    size = file.read(buffer, sizeof(buffer));
    if (size > 0) {
      client.write(buffer, size);
    }
  }

  file.close();
  client.stop();
  response->println("Screenshot sent over WiFi");
}
#endif

#endif
