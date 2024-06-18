/**
 * @file cli.cpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2024-06
 */

#ifndef DISABLE_CLI
#include "cli.hpp"
#include "utils.h"

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
  Pair<String, String> operands = wcli.parseCommand(args);
  String ip = operands.first();
  uint16_t port = operands.second().toInt();
 
  if (ip.isEmpty()){
    response->println("Saving to SD..");
    captureScreenshot(SCREENSHOT_TEMP_FILE, response);
    response->println("Note: is possible to send it to a PC using: scshot ip port");
  }
  else {
    response->printf("Sending screenshot to %s:%i..\r\n", ip.c_str(), port);
    captureScreenshot(SCREENSHOT_TEMP_FILE, response);
    captureScreenshot(SCREENSHOT_TEMP_FILE, ip.c_str(), port);
  }
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
  wcli.add("scshot", &wcli_scshot, "\tscreenshot to SD or sending a PC");
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
