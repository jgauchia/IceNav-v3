/*
       @file       8_Func_Batt.h
       @brief      Funciones para la batería

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// *********************************************
//  Función que lee voltaje de la batería
// *********************************************
float Read_Battery()
{
  float batt_read = round(((analogRead(34) * 1.81) / 1000)*10)/10.0;

  //return mapfloat(batt_read, 3.60, 4.20, 0, 100);
  return batt_read;
}

// *********************************************
//  Función que muestra icono batería y %
//
//  x,y:  Posición del indicador
// *********************************************
void show_battery(int x, int y)
{
  float average = 0;
  for (int i = 0; i < 1000; i++) 
  {
    average = average + batt;
  }
  average = average / 1000.0;
  sprintf(s_buf, "%03f%%", batt);
  tft.drawString(s_buf, x, y + 24, 1);
  if (average != batt_old)
  {
    tft.setSwapBytes(true);
    if (average > 80 && average <= 100 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_full_icon);
    else if (average <= 80 && average > 60 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_3_icon);
    else if (average <= 60 && average > 40 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_half_icon);
    else if (average <= 40 && average > 20 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_2_icon);
    else if (average <= 20 )
      tft.pushImage(x, y , Icon_Notify_Width, Icon_Notify_Height, battery_1_icon);
    batt_old = average;
    tft.setSwapBytes(false);
  }
}
