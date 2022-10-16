/**
 * @file sdcard.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  SD Card definition and functions
 * @version 0.1
 * @date 2022-10-09
 */

#include <FS.h>
#include <SD.h>

SPIClass spiSD = SPIClass();
bool sdloaded = false;

/**
 * @brief SD Card init
 * 
 */
void init_sd()
{
  spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spiSD, 10000000))
  {
    debug->println("Card Mount Failed");
    return;
  }
  else sdloaded = true;
}