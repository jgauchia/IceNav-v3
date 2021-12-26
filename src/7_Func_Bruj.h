/*
       @file       7_Func_Bruj.h
       @brief      Funciones para la brujula

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// *********************************************
//  Función que lee la brújula
// *********************************************
float Read_Mag_data()
{
  sensors_event_t event;
  mag.getEvent(&event);
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  heading += declinationAngle;
  if (heading < 0)
    heading += 2 * PI;
  if (heading > 2 * PI)
    heading -= 2 * PI;
  return heading;// * 180 / M_PI;
}


// **********************************************
//  Función principal que muestra la brujula
// **********************************************
void show_Compass()
{
  f_rumbo = Read_Mag_data();
  if (f_rumbo_temp != f_rumbo)
  {
    tft.drawLine(118, 207, 118 - (int)(45 * sin(f_rumbo_temp)), 207 - (int)(45 * cos(f_rumbo_temp)), TFT_WHITE);
    tft.drawLine(118, 207, 118 + (int)(45 * sin(f_rumbo_temp)), 207 + (int)(45 * cos(f_rumbo_temp)), TFT_WHITE);
    f_rumbo_temp = f_rumbo;
  }
  tft.drawLine(118, 207, 118 - (int)(45 * sin(f_rumbo)), 207 - (int)(45 * cos(f_rumbo)), TFT_RED);
  tft.drawLine(118, 207, 118 + (int)(45 * sin(f_rumbo)), 207 + (int)(45 * cos(f_rumbo)), TFT_BLACK);
  tft.fillCircle(118, 207, 5, TFT_DARKGREY);
}
