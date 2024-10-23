/**
 * @file cli.cpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2024-10
 */

#ifndef DISABLE_CLI
#include "cli.hpp"

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

void wcli_reboot(char *args, Stream *response)
{
  ESP.restart();
}

void wcli_poweroff(char *args, Stream *response) {
  deviceSuspend();
}

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

void wcli_swipe(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String deviceId = operands.first();
  response->println("Clearing device to defaults..");
  wcli.clearSettings();
  cfg.clear();
  response->println("done");
}

void wcli_clear(char *args, Stream *response)
{
  wcli.shell->clear();
}

void wcli_scshot(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String ip = operands.first();
  uint16_t port = operands.second().toInt();
 
  if (ip.isEmpty()){
    response->println("Saving to SD..");

    waitScreenRefresh = true;
    captureScreenshot(SCREENSHOT_TEMP_FILE, response);
    waitScreenRefresh = false;
    
    response->println("Note: is possible to send it to a PC using: scshot ip port");
  }
  else {
    if (!WiFi.isConnected()) {
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

  for (int i = key_count; i < PKEYS::KCOUNT; i++) {
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
  if(cfg.saveAuto(key,v)){
    response->printf("saved key %s\t: %s\r\n", key, v);
  }
}

void wcli_waypoint(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String path;
  String commands = operands.first();
  String fileDel = operands.second();
  String argsStr = args;
  String downArgs[4];
  int argsCnt = 0;

  if (commands.isEmpty())
  {
    response->println("");
    response->println(F( "\033[1;31m----\033[1;32m Available commands \033[1;31m----\033[0;37m\r\n" ));
    response->println(F( "\033[1;32mlist:\t\033[0;37mlist waypoints files"));
    response->println(F( "\033[1;32mdown:\t\033[0;37mdownload waypoint file"));
    response->println(F( "\033[1;32mdel:\t\033[0;37mdelete waypoint file"));
  }
  else if (commands.equals("list"))
  {
    acquireSdSPI();

    path = "/WPT";

    File dir = SD.open(path);

    response->println(F("\r\n\033[4mFile        \tSize\033[0m"));

    while (true) 
    {
      File entry =  dir.openNextFile();
      if (!entry) 
      {
        entry.close();
        break;
      }
      response->print(entry.name());
      response->print("\t");
      response->println(entry.size());
    }
    dir.close();

    releaseSdSPI();
  }
  else if (commands.equals("down"))
  {
    while (argsStr.length() > 0)
    {
      int index = argsStr.indexOf(' ');
      if (index == -1)
      {
        downArgs[argsCnt++] = argsStr;
        break;
      }
      else
      {
        downArgs[argsCnt++] = argsStr.substring(0, index);
        argsStr = argsStr.substring(index+1);
      }
    }
    if (downArgs[1].isEmpty())
      response->println(F("File name missing"));
    else if (downArgs[2].isEmpty())
      response->println(F("IP destination missing"));
    else if (downArgs[3].isEmpty())
      response->println(F("Port missing"));
    else
    {
      
      path = "/WPT/" + downArgs[1];

      response->println(path);

      response->printf("Sending screenshot to %s:%i..\r\n", downArgs[2].c_str(), downArgs[3].toInt());

      if (!client.connect(downArgs[2].c_str(), downArgs[3].toInt())) 
      {
        response->println("Connection to server failed");
        return;
      }

      response->println("Connected to server");
      
      acquireSdSPI();
      
      File file = SD.open(path, FILE_READ);
      if (!file)
      {
        response->println("Failed to open file for reading");
        client.stop();
        releaseSdSPI();
        return;
      }

      // Send the file data to the PC
      while (file.available())
      {
        size_t size = 0;
        uint8_t buffer[512];
        size = file.read(buffer, sizeof(buffer));
        if (size > 0)
          client.write(buffer, size);
      }

      file.close();
      client.stop();
      response->println("Waypoint file sent over WiFi");
      
      releaseSdSPI();
    }
  }
  else if (commands.equals("del"))
  {
    if (fileDel.isEmpty())
      response->println(F("File name missing"));
    else
    {
      acquireSdSPI();

      path = "/WPT/" + fileDel;
      if (!SD.remove(path))
      {
        response->print(F("Error deleting file "));
        response->println(fileDel);
      }
      else
      {
        response->print(F("File "));
        response->print(fileDel);
        response->println(F(" deleted"));
      }

      releaseSdSPI();
    }
  }
}

void wcli_settings(char *args, Stream *response)
{
  Pair<String, String> operands = wcli.parseCommand(args);
  String commands = operands.first();
  String value = operands.second();
  int8_t gpio = -1;

  if (commands.isEmpty())
  {
    response->println("");
    response->println(F( "\033[1;31m----\033[1;32m Available commands \033[1;31m----\033[0;37m\r\n" ));
    response->println(F( "\033[1;32msetgpstx:\t\033[0;37mset GPS Tx GPIO"));
    response->println(F( "\033[1;32msetgpsrx:\t\033[0;37mset GPS Rx GPIO"));
  }
  else if (commands.equals("setgpstx"))
  {
      if(value.isEmpty())
        response->println(F("Tx GPIO missing, use: setgpstx \033[1;32mGPIO\033[0;37m"));
      else
      {
        gpio = value.toInt();
        saveGpsGpio(gpio, -1);
        response->println("");
        response->printf("GPS \033[1;31mTx GPIO\033[0;37m set to: \033[1;32m%i\033[0;37m\r\n",gpio);
        response->println("Please reboot device");
      }
  }
  else if (commands.equals("setgpsrx"))
  {
      if(value.isEmpty())
        response->println(F("Rx GPIO missing, use: setgpsrx \033[1;32mGPIO\033[0;37m"));
      else
      {
        gpio = value.toInt();
        saveGpsGpio(-1, gpio);
        response->println("");
        response->printf("GPS \033[1;31mRx GPIO\033[0;37m set to: \033[1;32m%i\033[0;37m\r\n",gpio);
        response->println("Please reboot device");
      }
  }
}

void wcli_outnmea (char *args, Stream *response){
    nmea_output_enable = !nmea_output_enable;
}

void wcli_abort_handler () {
  if (nmea_output_enable) {
    nmea_output_enable = false;
    delay(100);
    Serial.println("\r\nCancel NMEA output!");
  } 
}

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
      saveWebFile(true);
      response->println("");
      response->printf("Web file server \033[1;32menabled\033[0;37m\r\n");
      response->println("Please reboot device");
    }
    if(commands.equals("disable"))
    {
      saveWebFile(false);
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

void initShell(){
  wcli.shell->attachLogo(logo);
  wcli.setSilentMode(true);
  // Main Commands:
  wcli.add("reboot", &wcli_reboot, "\tperform a ESP32 reboot");
  wcli.add("poweroff", &wcli_poweroff, "\tperform a ESP32 deep sleep");
  wcli.add("wipe", &wcli_swipe, "\t\twipe preferences to factory default");
  wcli.add("info", &wcli_info, "\t\tget device information");
  wcli.add("clear", &wcli_clear, "\t\tclear shell");
  wcli.add("scshot", &wcli_scshot, "\tscreenshot to SD or sending a PC");
  wcli.add("waypoint", &wcli_waypoint, "\twaypoint utilities");
  wcli.add("settings", &wcli_settings, "\tdevice settings");
  wcli.add("webfile", &wcli_webfile, "\tenable/disable Web file server");
  wcli.add("klist", &wcli_klist, "\t\tlist of user preferences. ('all' param show all)");
  wcli.add("kset", &wcli_kset, "\t\tset an user extra preference");
  wcli.add("outnmea", &wcli_outnmea, "\ttoggle GPS NMEA output (or Ctrl+C to stop)");
  wcli.shell->overrideAbortKey(&wcli_abort_handler);
  wcli.begin("IceNav");
}

/**
 * @brief WiFi CLI init and IceNav custom commands
 **/
void initCLI() 
{
  #ifndef ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);
  #endif
  log_v("init CLI");
  initShell(); 
  initRemoteShell();
}

#endif
