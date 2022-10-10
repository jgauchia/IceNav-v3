/**
 * @file notify_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Display notify bar
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display notify bar at x,y position
 * 
 * @param x -> X Position
 * @param y -> Y Position
 */
void show_notify_bar(int x, int y)
{
  show_sat_hour(x, y, 4);
  show_batt_icon(x + 200, y - 10);
  show_sat_icon(x + 170, y - 10);
}
