/**
 * @file sdcard.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  SD Card definition and functions
 * @version 0.1.8
 * @date 2024-04
 */

#include <FS.h>
#include <SD.h>

#ifdef MAKERF_ESP32S3
SPIClass spiSD = SPIClass(HSPI); 
uint32_t sdFreq = 10000000;
#else
SPIClass spiSD = SPIClass(VSPI);
uint32_t sdFreq = 40000000;
#endif
bool sdloaded = false;

/**
 * @brief SD Card init
 *
 */
void initSD()
{
  spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,LOW);
  if (!SD.begin(SD_CS, spiSD, sdFreq))
  {
    log_e("SD Card Mount Failed");
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
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
    log_e("SPIFFS Mount Failed");
  else
    log_v("SPIFFS Mounted");
}