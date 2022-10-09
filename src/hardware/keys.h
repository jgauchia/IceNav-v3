/**
 * @file keys.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  key inputs definition and functions
 * @version 0.1
 * @date 2022-10-09
 */
#ifdef ENABLE_PCF8574

#include <PCF8574.h>

PCF8574 keyboard(0x20);
enum Keys
{
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    PUSH,
    LUP,
    LBUT,
    LDOWN,
    BLEFT,
    BRIGHT
};
int key_pressed = NONE;

/**
 * @brief Keyboard refresh delay
 *
 */
#define KEYS_UPDATE_TIME 175
MyDelay KEYStime(KEYS_UPDATE_TIME);

/**
 * @brief Read keys
 * 
 * @return int 
 */
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

/**
 * @brief Keys actions
 * 
 * @param read_key 
 * @return * void 
 */
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

#endif