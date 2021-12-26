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
    case 254:
      return LEFT;
      break;
    case 253:
      return PUSH;
      break;
    case 251:
      return UP;
      break;
    case 247:
      return RIGHT;
      break;
    case 239:
      return DOWN;
      break;
    case 191:
      return BLEFT;
      break;
    case 223:
      return BRIGHT;
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
  if (read_key == BLEFT && !is_menu_screen)
  {
    is_draw = false;
    is_menu_screen = true;
  }
  else if (read_key == BLEFT && is_menu_screen)
  {
    is_draw = false;
    is_menu_screen = false;
  }
}
