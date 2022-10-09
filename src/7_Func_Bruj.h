/*
       @file       7_Func_Bruj.h
       @brief      Funciones para la brujula

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// **********************************************
//  Función principal que muestra la brujula
// **********************************************
void show_Compass()
{
  rumbo = Read_Mag_data();
  compass_sprite.pushRotated(360-rumbo,TFT_BLACK); 
  tft.setTextColor(TFT_BLACK,TFT_WHITE);
  tft.fillRect(55,207,130,40,TFT_WHITE);
  
  if ( key_pressed == PUSH && is_show_degree)
      is_show_degree = false;
  else if ( key_pressed == PUSH && !is_show_degree)
      is_show_degree = true;

  if ( !is_show_degree )
  {
    int altura = (int)GPS.altitude.meters();
    if (altura < 10)
      sprintf(s_buf, "%s%1d","      " , altura);
    else if (altura < 100)
      sprintf(s_buf, "%s%2d","    " , altura);
    else if (altura < 1000)
      sprintf(s_buf, "%s%3d","  " , altura);
    else
      sprintf(s_buf, "%4d", altura);
    tft.drawString(s_buf, 55, 207, 6);
    tft.drawString("m",165, 225, 4);
  }
  else
  {
    if (rumbo < 10)
     sprintf(s_buf,"%s%1d","    ", rumbo);
    else if (rumbo < 100)
     sprintf(s_buf,"%s%2d","  ", rumbo);
    else
     sprintf(s_buf, "%3d", rumbo);
    tft.drawString(s_buf,75,207,6);
    tft.setTextFont(4);
    tft.setCursor(165, 207);
    tft.print("`");
  }
}
