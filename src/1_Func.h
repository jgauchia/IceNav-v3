/*
       @file       1_Func.h
       @brief      Funciones necesarias para el programa

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Función para inicializar GPS
// **********************************************
void init_gps()
{
  gps->begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // Inicializar parse custom para tracking satelites
  for (int i = 0; i < 4; ++i)
  {
    satNumber[i].begin(GPS, "GPGSV", 4 + 4 * i); // offsets 4, 8, 12, 16
    elevation[i].begin(GPS, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
    azimuth[i].begin(GPS, "GPGSV", 6 + 4 * i);   // offsets 6, 10, 14, 18
    snr[i].begin(GPS, "GPGSV", 7 + 4 * i);       // offsets 7, 11, 15, 19
  }
}

// **********************************************
//  Función para salida monitor GPS
// **********************************************
void gps_out_monitor()
{
#ifdef OUTPUT_NMEA
  if (gps->available())
  {
    debug->println(GPS.location.lat());
  }
#endif
}

// **********************************************
//  Función para inicializar el LCD
// **********************************************
void init_ili9341()
{
  tft.init();
#ifdef CUSTOMBOARD
  tft.setRotation(2);
#endif

#ifdef TDISPLAY
  tft.setRotation(4);
#endif

  tft.fillScreen(TFT_BLACK);
  tft.initDMA();
}

// **********************************************
//  Función para inicializar el navegador
// **********************************************
void init_icenav()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  esp_wifi_stop();
  esp_bt_controller_disable();
  is_menu_screen = false;
  is_main_screen = true;
  keyboard.begin();
  mag.begin();
  KEYStime.start();
  BATTtime.start();
  COMPASStime.start();
  batt_level = Read_Battery();
  millis_actual = millis();
  tft.writecommand(0x28);
  drawBmp("/INIT.BMP", 0, 0, true);
  tft.writecommand(0x29);
  while (millis() < millis_actual + 4000);
  tft.fillScreen(TFT_BLACK);
#ifdef SEARCH_SAT_ON_INIT
  search_init_sat();
#endif
}
