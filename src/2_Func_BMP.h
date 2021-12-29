/*
       @file       2_Func_BMP.h
       @brief      Funciones necesarias para cargar imagens BMP desde tarjeta SD o SPIFFS

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Función para leer BMP y mostrar en pantalla
//
//  filename: Archivo BMP
//  x,y:      posición en pantalla
//  microsd:  bool, true = SD, false = SPIFFS
// **********************************************
void drawBmp(const char *filename, int16_t x, int16_t y, bool microsd)
{
  if ((x >= tft.width()) || (y >= tft.height())) return;

  File bmpFS;

  // Open requested file on SD card
  if (microsd)
    bmpFS = SD.open(filename);
  else
    SPIFFS.open(filename, "r");


  if (!bmpFS)
  {
    debug->print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t  r, g, b;

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {

        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else debug->println("BMP format not recognized.");
  }
  bmpFS.close();
}

// **********************************************
//  Funciones para leer 16 o 32 Bytes de la SD
// **********************************************
uint16_t read16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}
uint32_t read32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
