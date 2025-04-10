/**
 * @file cli.cpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2025-04
 */

#ifndef DISABLE_CLI
#include "cli.hpp"

static const char logo[] PROGMEM =
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

static const char* TAG PROGMEM = "CLI";

extern Power power;

/**
 * @brief Reboot ESP
 */
void wcli_reboot(char *args, Stream *response)
{
  ESP.restart();
}

/**
 * @brief ESP Deep Sleep/shutdown
 */
void wcli_poweroff(char *args, Stream *response)
{
  power.deviceShutdown();
}

/**
 * @brief Display device info
 */
void wcli_info(char *args, Stream *response)
{
  setlocale(LC_NUMERIC, "");
  size_t totalSPIFFS, usedSPIFFS, freeSPIFFS = 0;
  esp_spiffs_info(NULL, &totalSPIFFS, &usedSPIFFS);
  freeSPIFFS = totalSPIFFS - usedSPIFFS;

  response->println();
  wcli.status(response);
  response->printf("Total Memory\t: %3.0iKb\r\n",ESP.getHeapSize()/1000);
  response->printf("SPIFFS total\t: %u bytes\r\n", totalSPIFFS);
  response->printf("SPIFFS used\t: %u bytes\r\n", usedSPIFFS);
  response->printf("SPIFFS free\t: %u bytes\r\n", freeSPIFFS);
  if (psramFound())
  {
    response->printf("PSRAM total\t: %u bytes\r\n", ESP.getPsramSize());
    response->printf("PSRAM used\t: %u bytes\r\n", ESP.getPsramSize()-ESP.getFreePsram());
    response->printf("PSRAM free\t: %u bytes\r\n", ESP.getFreePsram());
  }
  response->printf("Flash size\t: %u bytes\r\n", ESP.getFlashChipSize());
  response->printf("Program size\t: %u bytes\r\n", ESP.getSketchSize());
  if (enableWeb)
    response->println("Web file server\t: \033[1;32menabled\033[0;37m");
  else
    response->println("Web file server\t: \033[1;31mdisabled\033[0;37m");
  response->printf("\r\n");
  response->printf("GPS Baud rate\t: %i baud\r\n",gpsBaudDetected);
  response->printf("GPS Tx GPIO:\t: %i\r\n",GPS_TX);
  response->printf("GPS Rx GPIO:\t: %i\r\n",GPS_RX);
}

/**
 * @brief Clear user settings
 */
void wcli_swipe(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String deviceId = operands.first();
  response->println("Clearing device to defaults..");
  wcli.clearSettings();
  cfg.clear();
  response->println("done");
}

/**
 * @brief Clear CLI console
 */
void wcli_clear(char *args, Stream *response)
{
  wcli.shell->clear();
}

/**
 * @brief Take a screenshot
 */
void wcli_scshot(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String ip = operands.first();
  uint16_t port = operands.second().toInt();
 
  if (ip.isEmpty())
  {
    response->println("Saving to SD..");

    waitScreenRefresh = true;
    captureScreenshot(SCREENSHOT_TEMP_FILE, response);
    waitScreenRefresh = false;
    
    response->println("Note: is possible to send it to a PC using: scshot ip port");
  }
  else
  {
    if (!WiFi.isConnected()) 
    {
      response->println("Please connect your WiFi first!");
      return;
    }
    response->printf("Sending screenshot to %s:%i..\r\n", ip.c_str(), port);

    waitScreenRefresh = true;
    captureScreenshot(SCREENSHOT_TEMP_FILE, ip.c_str(), port, response);
    waitScreenRefresh = false;
  }
}

/**
 * @brief list of user preference key. This depends of EasyPreferences manifest.
 * @author @Hpsaturn. Method migrated from CanAirIO project
 */
void wcli_klist(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String opt = operands.first();
  int key_count = PKEYS::KUSER+1;
  if (opt.equals("all")) key_count = 0; // Only show the basic keys to configure
  response->printf("\n%11s \t%s \t%s \r\n", "KEYNAME", "DEFINED", "VALUE");
  response->printf("\n%11s \t%s \t%s \r\n", "=======", "=======", "=====");

  for (int i = key_count; i < PKEYS::KCOUNT; i++)
  {
    if (i == PKEYS::KUSER) continue;
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = cfg.getValue(key);
    response->printf("%11s \t%s \t%s \r\n", key, defined.c_str(), value.c_str());
  }
}

/**
 * @brief set an user preference key. This depends of EasyPreferences manifest.
 * @author @Hpsaturn. Method migrated from CanAirIO project
 */
void wcli_kset(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String key = operands.first();
  String v = operands.second();
  if (cfg.saveAuto(key,v))
    response->printf("saved key %s\t: %s\r\n", key, v);
}


/**
 * @brief Output NMEA sentences in CLI
 */
void wcli_outnmea(char *args, Stream *response)
{
  nmea_output_enable = !nmea_output_enable;
}

/**
 * @brief Cancel NMEA Output
 */
void wcli_abort_handler() 
{
  if (nmea_output_enable)
  {
    nmea_output_enable = false;
    delay(100);
    Serial.println("\r\nCancel NMEA output!");
  } 
}

/**
 * @brief Webfile server enable/disable option
 */
void wcli_webfile(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String commands = operands.first();

  if (commands.isEmpty())
    response->println(F("missing parameter use: webfile \033[1;32menable/disable\033[0;37m"));
  else
  {
    if(commands.equals("enable"))
    {
      cfg.saveBool(PKEYS::KWEB_FILE, true);
      response->println("");
      response->printf("Web file server \033[1;32menabled\033[0;37m\r\n");
      response->println("Please reboot device");
    }
    if(commands.equals("disable"))
    {
      cfg.saveBool(PKEYS::KWEB_FILE, false);
      response->println("");
      response->printf("Web file server \033[1;32mdisabled\033[0;37m\r\n");
      response->println("Please reboot device");
    }
  }
}

void initRemoteShell()
{
#ifndef DISABLE_CLI_TELNET 
  if (wcli.isTelnetRunning()) wcli.shellTelnet->attachLogo(logo);
#endif
}

void initShell()
{
  wcli.shell->attachLogo(logo);
  wcli.setSilentMode(true);
  // Main Commands:
  wcli.add("reboot", &wcli_reboot, "\tperform a ESP32 reboot");
  wcli.add("poweroff", &wcli_poweroff, "\tperform a ESP32 deep sleep");
  wcli.add("wipe", &wcli_swipe, "\t\twipe preferences to factory default");
  wcli.add("info", &wcli_info, "\t\tget device information");
  wcli.add("clear", &wcli_clear, "\t\tclear shell");
  wcli.add("scshot", &wcli_scshot, "\tscreenshot to SD or sending a PC");
  wcli.add("webfile", &wcli_webfile, "\tenable/disable Web file server");
  wcli.add("klist", &wcli_klist, "\t\tlist of user preferences. ('all' param show all)");
  wcli.add("kset", &wcli_kset, "\t\tset an user extra preference");
  wcli.add("outnmea", &wcli_outnmea, "\ttoggle GPS NMEA output (or Ctrl+C to stop)");
  wcli.shell->overrideAbortKey(&wcli_abort_handler);
  wcli.begin("IceNav");
}

/**
 * @brief WiFi CLI init
 **/
void initCLI() 
{
  Serial.begin(115200);
  ESP_LOGV(TAG, "init CLI");
  initShell(); 
  initRemoteShell();
}

#endif
