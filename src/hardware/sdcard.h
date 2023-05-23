/**
 * @file sdcard.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  SD Card definition and functions
 * @version 0.1.4
 * @date 2023-05-23
 */

#include <FS.h>
#include <SD.h>

#ifdef MAKERF_ESP32S3
SPIClass spiSD = SPIClass(HSPI); 
#else
SPIClass spiSD = SPIClass(VSPI);
#endif
bool sdloaded = false;

/**
 * @brief SD Card init
 *
 */
void init_sd()
{
  spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,LOW);
  if (!SD.begin(SD_CS, spiSD, 8000000))
  {
    debug->println(PSTR("SD Card Mount Failed"));
    return;
  }
  else
  {
    debug->println(PSTR("SD Card Mounted"));
    sdloaded = true;
  }
}

/**
 * @brief SPIFFS Init
 *
 */
void init_SPIFFS()
{
  if (!SPIFFS.begin(true))
    debug->println(PSTR("SPIFFS Mount Failed"));
  else
    debug->println(PSTR("SPIFFS Mounted"));
}