/**
 * @file timezone.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Time zone adjust
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef TIMEZONE_HPP
#define TIMEZONE_HPP

extern int32_t defGMT;          // Default GMT offset
extern String defDST;           // Default DST zone
extern bool calculateDST;       // Calculate DST flag

//  Calculate DST changeover times once per reset and year!
static NeoGPS::time_t  changeover;
static NeoGPS::clock_t springForward, fallBack;

static uint8_t springMonth =  0;
static uint8_t springDate  =  0; // latest last Sunday
static uint8_t springHour  =  0;
static uint8_t fallMonth   =  0;
static uint8_t fallDate    =  0; // latest last Sunday
static uint8_t fallHour    =  0;

/**
 * @brief Adjust Time (GMT and DST)
 *
 * @param dt -> GPS Time
 */
static void adjustTime( NeoGPS::time_t & dt )
{
  NeoGPS::clock_t seconds = dt; // convert date/time structure to seconds

  // Set these values to the offset of your timezone from GMT
  static const int32_t          zone_minutes =  0L; // usually zero
  static const NeoGPS::clock_t  zone_offset = (int32_t)defGMT * NeoGPS::SECONDS_PER_HOUR +
                                              zone_minutes * NeoGPS::SECONDS_PER_MINUTE;

  if (calculateDST)
  {
    if (defDST.equals("USA"))
    {
      springMonth =  3;
      springDate  = 14; // latest 2nd Sunday
      springHour  =  2;
      fallMonth   = 11;
      fallDate    =  7; // latest 1st Sunday
      fallHour    =  2;
    }
    if (defDST.equals("EU"))
    {
      springMonth =  3;
      springDate  = 31; // latest last Sunday
      springHour  =  2;
      fallMonth   = 10;
      fallDate    = 31; // latest last Sunday
      fallHour    =  1;
    }

    if ((springForward == 0) || (changeover.year != dt.year))
    {

      //  Calculate the spring changeover time (seconds)
      changeover.year    = dt.year;
      changeover.month   = springMonth;
      changeover.date    = springDate;
      changeover.hours   = springHour;
      changeover.minutes = 0;
      changeover.seconds = 0;
      changeover.set_day();
      // Step back to a Sunday, if day != SUNDAY
      changeover.date -= (changeover.day - NeoGPS::time_t::SUNDAY);
      springForward = (NeoGPS::clock_t) changeover;

      //  Calculate the fall changeover time (seconds)
      changeover.month   = fallMonth;
      changeover.date    = fallDate;
      changeover.hours   = fallHour - 1; // to account for the "apparent" DST +1
      changeover.set_day();
      // Step back to a Sunday, if day != SUNDAY
      changeover.date -= (changeover.day - NeoGPS::time_t::SUNDAY);
      fallBack = (NeoGPS::clock_t) changeover;
    }
  }

  //  First, offset from UTC to the local timezone
  seconds += zone_offset;

  if (calculateDST)
  {
    //  Then add an hour if DST is in effect
    if ((springForward <= seconds) && (seconds < fallBack))
      seconds += NeoGPS::SECONDS_PER_HOUR;
  }

  dt = seconds; // convert seconds back to a date/time structure

} 

/**
 * @brief convert hour to HH:MM rounded
 *
 * @param h -> hour
 * @param str -> HH:MM text format
 */
static char * hoursToString(double h, char *str)
{
  int m = int(round(h * 60));
  int hr = (m / 60) % 24;
  int mn = m % 60;

  str[0] = (hr / 10) % 10 + '0';
  str[1] = (hr % 10) + '0';
  str[2] = ':';
  str[3] = (mn / 10) % 10 + '0';
  str[4] = (mn % 10) + '0';
  str[5] = '\0';
  return str;
}

#endif