/**
 * @file sdcard.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  SD Card definition and functions
 * @version 0.1.5
 * @date 2023-06-04
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
  if (!SD.begin(SD_CS, spiSD, 10000000))
  {
    log_v("SD Card Mount Failed");
    return;
  }
  else
  {
    log_v("SD Card Mounted");
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
    log_v("SPIFFS Mount Failed");
  else
    log_v("SPIFFS Mounted");
}