/*
       @file       8_Func_Batt.h
       @brief      Funciones para la batería

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// *********************************************
//  Función que lee voltaje de la batería
// *********************************************
int Read_Battery()
{
  unsigned long start = millis();
  do
  {
    return batt.getBatteryChargeLevel(true);
  } while (millis() - start < BATT_UPDATE_TIME);
}

// *********************************************
//  Función que muestra icono batería y %
//
//  x,y:  Posición del indicador
// *********************************************
void show_battery(int x, int y)
{
  if (batt_level != old_batt_level)
  {
    tft.setSwapBytes(true);
    if (batt_level > 80 && batt_level <= 100 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_full_icon);
    else if (batt_level <= 80 && batt_level > 60 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_3_icon);
    else if (batt_level <= 60 && batt_level > 40 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_half_icon);
    else if (batt_level <= 40 && batt_level > 20 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_2_icon);
    else if (batt_level <= 20 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_1_icon);
    old_batt_level = batt_level;
    tft.setSwapBytes(false);
    sprintf(s_buf, "%d%%", old_batt_level);
    tft.drawString(s_buf, x, y + 24, 1);
  }
}
