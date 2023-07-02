/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass definition and functions
 * @version 0.1.6
 * @date 2023-06-14
 */

#ifdef CUSTOMBOARD
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(12345);
#endif

#ifdef MAKERF_ESP32S3
#include <MPU9250.h>

MPU9250 IMU(Wire, 0x68);
#endif

/**
 * @brief Magnetic declination
 *
 */
//  Obtain your magnetic declination from http://www.magnetic-declination.com/
//  By convention, declination is positive when magnetic north
//  is east of true north, and negative when it is to the west.
//  Substitute your magnetic declination for the "declinationAngle" shown below.
float declinationAngle = 0.22;

/**
 * @brief Compass Heading Angle
 *
 */
int heading = 0;
float heading_smooth = 0.0;
float heading_previous = 0.0;
#define SMOOTH_FACTOR 0.40
#define SMOOTH_PREVIOUS_FACTOR 0.60

/**
 * @brief Read compass data
 *
 * @return compass heading
 */
int read_compass()
{
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;

#ifdef CUSTOMBOARD
  sensors_event_t event;
  compass.getEvent(&event);
  y = event.magnetic.y;
  x = event.magnetic.x;
  z = event.magnetic.z;
#endif

#ifdef MAKERF_ESP32S3
  IMU.readSensor();
  x = IMU.getMagX_uT();
  y = IMU.getMagY_uT();
  z = IMU.getMagZ_uT();
#endif
  float heading_no_filter = atan2(y, x);
  heading_no_filter += declinationAngle;
  //heading_smooth = heading_no_filter;
  heading_smooth = (heading_no_filter * SMOOTH_FACTOR) + (heading_previous * SMOOTH_PREVIOUS_FACTOR);
  heading_previous = heading_smooth;

  if (heading_smooth < 0)
    heading_smooth += 2 * M_PI;
  if (heading_smooth > 2 * M_PI)
    heading_smooth -= 2 * M_PI;
  return (int)(heading_smooth * 180 / M_PI);
}

void init_compass()
{

#ifdef CUSTOMBOARD
  compass.begin();
#endif
#ifdef MAKERF_ESP32S3
  int status = IMU.begin();
  if (status < 0)
  {
    log_e("IMU initialization unsuccessful");
    log_e("Check IMU wiring or try cycling power");
    log_e("Status: %i", status);
  }
#endif
}
