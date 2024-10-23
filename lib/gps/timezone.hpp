/**
 * @file timezone.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Time zone adjust
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef TIMEZONE_HPP
#define TIMEZONE_HPP

// Set these values to the offset of your timezone from GMT

static const int32_t          zone_hours   =  1L; 
static const int32_t          zone_minutes =  0L; // usually zero
static const NeoGPS::clock_t  zone_offset  =
                              zone_hours   * NeoGPS::SECONDS_PER_HOUR +
                              zone_minutes * NeoGPS::SECONDS_PER_MINUTE;


// Uncomment one DST changeover rule, or define your own:

//#define USA_DST
#define EU_DST

#if defined(USA_DST)
  static const uint8_t springMonth =  3;
  static const uint8_t springDate  = 14; // latest 2nd Sunday
  static const uint8_t springHour  =  2;
  static const uint8_t fallMonth   = 11;
  static const uint8_t fallDate    =  7; // latest 1st Sunday
  static const uint8_t fallHour    =  2;
  #define CALCULATE_DST

#elif defined(EU_DST)
  static const uint8_t springMonth =  3;
  static const uint8_t springDate  = 31; // latest last Sunday
  static const uint8_t springHour  =  2;
  static const uint8_t fallMonth   = 10;
  static const uint8_t fallDate    = 31; // latest last Sunday
  static const uint8_t fallHour    =  1;
  #define CALCULATE_DST
#endif

/**
 * @brief Update notify bar event
 *
 * @param dt -> GPS Time
 */
static void adjustTime( NeoGPS::time_t & dt )
{
  NeoGPS::clock_t seconds = dt; // convert date/time structure to seconds

  #ifdef CALCULATE_DST
    //  Calculate DST changeover times once per reset and year!
    static NeoGPS::time_t  changeover;
    static NeoGPS::clock_t springForward, fallBack;

    if ((springForward == 0) || (changeover.year != dt.year)) {

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
  #endif

  //  First, offset from UTC to the local timezone
  seconds += zone_offset;

  #ifdef CALCULATE_DST
    //  Then add an hour if DST is in effect
    if ((springForward <= seconds) && (seconds < fallBack))
      seconds += NeoGPS::SECONDS_PER_HOUR;
  #endif

  dt = seconds; // convert seconds back to a date/time structure

} 
#endif