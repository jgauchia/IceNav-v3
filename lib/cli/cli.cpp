/**
 * @file cli.cpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2024-06
 */

#ifndef DISABLE_CLI
#include "cli.hpp"
#include "storage.hpp"
#include "tft.hpp"

const char logo[] =
"\r\n"
"░▒▓█▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ \r\n"
"░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ \r\n"
"░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒▒▓█▓▒░  \r\n"
"░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓██████▓▒░   ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓████████▓▒░  ░▒▓█▓▒▒▓█▓▒░  \r\n"
"░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░   ░▒▓█▓▓█▓▒░   \r\n"
"░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░   ░▒▓█▓▓█▓▒░   \r\n"
"░▒▓█▓▒░  ░▒▓██████▓▒░  ░▒▓████████▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░    ░▒▓██▓▒░    \r\n"
"\r\n"
""
;

// Capture the screenshot and save it to the SD card
void captureScreenshot(const char* filename, Stream *response) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    response->println("Failed to open file for writing");
    return;
  }

  // Allocate memory to store the screen data (1 byte per segment)
  uint8_t* buffer = (uint8_t*)malloc(tft.width() * tft.height() * 2); // 2 bytes per pixel
  if (!buffer) {
    response->println("Failed to allocate memory for buffer");
    file.close();
    return;
  }

  // Read the screen data into the buffer using readRect
  tft.readRect(0, 0, tft.width(), tft.height(), (uint16_t*)buffer);

  // Write the buffer data to the file
  for (int y = 0; y < tft.height(); y++) {
    for (int x = 0; x < tft.width(); x++) {
      // Combine the two 8-bit segments into a 16-bit value
      uint8_t highByte = buffer[(y * tft.width() + x) * 2];
      uint8_t lowByte = buffer[(y * tft.width() + x) * 2 + 1];
      uint16_t color = (highByte << 8) | lowByte;
      file.write((const uint8_t*)&color, sizeof(color));
    }
  }

  // Clean up
  free(buffer);
  file.close();
  response->println("Screenshot saved");
}

void wcli_reboot(char *args, Stream *response) {
  ESP.restart();
} 

void wcli_info(char *args, Stream *response) {
  response->println();
  wcli.status(response);
}

void wcli_clear(char *args, Stream *response){
  wcli.shell->clear();
}

void wcli_scshot(char *args, Stream *response){
  captureScreenshot("/screenshot.raw",response);
}

void initRemoteShell(){
#ifndef DISABLE_CLI_TELNET 
  if (wcli.isTelnetEnable()) wcli.shellTelnet->attachLogo(logo);
#endif
}

void initShell(){
  wcli.shell->attachLogo(logo);
  wcli.setSilentMode(true);
  wcli.disableConnectInBoot();
  // Main Commands:
  wcli.add("reboot", &wcli_reboot, "\tperform a ESP32 reboot");
  wcli.add("info", &wcli_info, "\t\tget device information");
  wcli.add("clear", &wcli_clear, "\t\tclear shell");
  wcli.add("scshot", &wcli_scshot, "\ttake screen shot");
  wcli.begin("IceNav");
}

/**
 * @brief WiFi CLI init and IceNav custom commands
 **/
void initCLI() {
  Serial.begin(115200);
  #ifdef ARDUINO_USB_CDC_ON_BOOT
    while (!Serial){};
    delay(1000);
  #endif
  log_v("init CLI");
  initShell(); 
  initRemoteShell();
}

#endif
