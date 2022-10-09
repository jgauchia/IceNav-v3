// **********************************************
//  Función para inicializar el LCD
// **********************************************
void init_tft()
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
#ifdef DISABLE_RADIO
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  esp_wifi_stop();
  esp_bt_controller_disable();
#endif
  is_menu_screen = false;
  is_main_screen = true;
#ifdef ENABLE_PCF8574
  keyboard.begin();
  KEYStime.start();
#endif
#ifdef ENABLE_COMPASS
  compass.begin();
  COMPASStime.start();
#endif

  BATTtime.start();
  batt_level = Read_Battery();
  millis_actual = millis();
  tft.writecommand(0x28);
  drawBmp("/INIT.BMP", 0, 0, true);
  tft.writecommand(0x29);
  while (millis() < millis_actual + 4000)
    ;
  tft.fillScreen(TFT_BLACK);
#ifdef SEARCH_SAT_ON_INIT
  search_sat_scr();
#endif
}
