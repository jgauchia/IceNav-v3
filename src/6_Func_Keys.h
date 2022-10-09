/*
       @file       6_Func_Keys.h
       @brief      Funciones para las teclas

       @author     Jordi Gauchia

       @date       08/12/2021
*/

// *********************************************
//  Función que lee las teclas del PCF8574
// *********************************************
int Read_Keys()
{
  keyboard.read8();
  switch (keyboard.value())
  {
    case 223:
      return LUP;
      break;
    case 191:
      return LBUT;
      break;
    case 127:
      return LDOWN;
      break;
    case 239:
      return UP;
      break;
    case 254:
      return DOWN;
      break;
    case 251:
      return LEFT;
      break;
    case 253:
      return RIGHT;
      break;
    case 247:
      return PUSH;
      break;
    default:
      return NONE;
      break;
  }
}

// *********************************************
//  Función que comprueba las pulsaciones del
//  teclado y en función de la opción de menú
//  realiza acciones
//
//  key:     tecla leída
// *********************************************
void Check_keys(int read_key)
{
  if (read_key == PUSH && !is_menu_screen)
  {
    is_draw = false;
    is_menu_screen = true;
    is_main_screen = false;
  }
  else if (read_key == PUSH && is_menu_screen)
  {
    is_draw = false;
    is_menu_screen = false;
    is_main_screen = true;
  }

  if (read_key == LUP && is_map_screen)
  {
    zoom++;
    if (zoom > MAX_ZOOM)
       zoom = MAX_ZOOM;
  }
  else if (read_key == LDOWN && is_map_screen)
  {
    zoom--;
    if (zoom < MIN_ZOOM)
       zoom = MIN_ZOOM;
  }

  if (read_key == RIGHT && is_main_screen)
  {
    is_draw = false;
    sel_MainScreen++;
    if (sel_MainScreen > MAX_MAIN_SCREEN)
      sel_MainScreen = 1;
  }
  else if (read_key == LEFT && is_main_screen)
  {
    is_draw = false;
    sel_MainScreen--;
    if (sel_MainScreen < 1)
      sel_MainScreen = MAX_MAIN_SCREEN;
  }
}
