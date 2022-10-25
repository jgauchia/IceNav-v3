/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Display map tile
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Display map tile at x,y postion
 * 
 * @param posx -> X position
 * @param posy -> Y position
 * @param lon  -> longitude
 * @param lat  -> latitude
 */
void show_map(int posx, int posy, double lon, double lat)
{
  char s_fichmap[40];
  int x = lon2tilex(lon, zoom);
  int y = lat2tiley(lat, zoom);
  tft.fillCircle(lon2posx(lon, zoom) + posx, lat2posy(lat, zoom) + posy, 2, TFT_RED);
  if (zoom != zoom_old || (x != tilex || y != tiley))
  {
    tilex = x;
    tiley = y;
    sprintf(s_fichmap, "/MAP/%d/%d/%d.png", zoom, tilex, tiley);
    setPngPosition(posx, posy);
    load_file(SD, s_fichmap);
    debug->println(s_fichmap);
    zoom_old = zoom;
  }
}
