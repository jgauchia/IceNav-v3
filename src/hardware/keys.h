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
#endif

#include "hardware/keys_def.h"
int buttonState;
int lastButtonState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;

/**
 * @brief Read keys
 *
 * @return int -> enum structure keys index
 */
static int Read_Keys()
{
#ifdef ENABLE_PCF8574
  keyboard.read8();
  int reading = keyboard.value();
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      switch (buttonState)
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
  }
  lastButtonState = reading;
  return NONE;
#endif
}
