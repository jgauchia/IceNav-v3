/**
 * @file cli.cpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2024-08
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

void wcli_info(char *args, Stream *response)
{
  response->println();
  wcli.status(response);
  response->print("GPS Baud rate\t: ");
  response->println(gpsBaudDetected);
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
    response->printf("Sending screenshot to %s:%i..\r\n", ip.c_str(), port);

    waitScreenRefresh = true;
    captureScreenshot(SCREENSHOT_TEMP_FILE, ip.c_str(), port, response);
    waitScreenRefresh = false;
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

void initRemoteShell()
{
#ifndef DISABLE_CLI_TELNET 
  if (wcli.isTelnetEnable()) wcli.shellTelnet->attachLogo(logo);
#endif
}

void initShell(){
  wcli.shell->attachLogo(logo);
  wcli.setSilentMode(true);
  // Main Commands:
  wcli.add("reboot", &wcli_reboot, "\tperform a ESP32 reboot");
  wcli.add("wipe", &wcli_swipe, "\t\twipe preferences to factory default");
  wcli.add("info", &wcli_info, "\t\tget device information");
  wcli.add("clear", &wcli_clear, "\t\tclear shell");
  wcli.add("scshot", &wcli_scshot, "\tscreenshot to SD or sending a PC");
  wcli.add("waypoint", &wcli_waypoint, "\twaypoint utilities");
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
