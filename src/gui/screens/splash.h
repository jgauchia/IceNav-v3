/**
 * @file splash.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Splash screen
 * 
 */
void splash_scr()
{
    tft.writecommand(0x28);
    drawBmp("/INIT.BMP", 0, 0, true);
    tft.writecommand(0x29);
}