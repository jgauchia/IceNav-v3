/**
 * @file compass.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Compass definition and functions
 * @version 0.1.8
 * @date 2024-04
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

#define COMPASS_CAL_TIME 16000
static void saveCompassCal(float offsetX, float offsetY);

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
 * @brief Compass Heading Angle and Smooth factors
 *
 */
int heading = 0;
int mapHeading = 0;
float headingSmooth = 0.0;
float headingPrevious = 0.0;
#define SMOOTH_FACTOR 0.40
#define SMOOTH_PREVIOUS_FACTOR 0.60

/**
 * @brief Calibration variables
 *
 */
float minX, maxX, minY, maxY;

/**
 * @brief Init Compass
 *
 */
void initCompass()
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

/**
 * @brief Read compass values
 *
 * @param x
 * @param y
 * @param z
 */
static void readCompass(float &x, float &y, float &z)
{
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
}

/**
 * @brief Get compass heading
 *
 * @return compass heading
 */
int getHeading()
{
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;

  readCompass(x, y, z);

  float headingNoFilter = atan2(y - offY, x - offX);
  headingNoFilter += declinationAngle;
  headingSmooth = headingNoFilter;
  // headingSmooth = (headingNoFilter * SMOOTH_FACTOR) + (headingPrevious * SMOOTH_PREVIOUS_FACTOR);
  // headingPrevious = headingSmooth;

  if (headingSmooth < 0)
    headingSmooth += 2 * M_PI;
  if (headingSmooth > 2 * M_PI)
    headingSmooth -= 2 * M_PI;
  return (int)(headingSmooth * 180 / M_PI);
}

/**
 * @brief Compass calibration
 *
 */
static void compassCalibrate()
{
  bool cal = 1;
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;
  uint16_t touchX, touchY;

  tft.drawCenterString("ROTATE THE DEVICE", 160, 10, &fonts::DejaVu18);
  tft.drawPngFile(SPIFFS, PSTR("/turn.png"), (tft.width() / 2) - 50, 60);
  tft.drawCenterString("TOUCH TO START", 160, 200, &fonts::DejaVu18);
  tft.drawCenterString("COMPASS CALIBRATION", 160, 230, &fonts::DejaVu18);

  while (!tft.getTouch(&touchX, &touchY))
  {
  };
  delay(1000);

  unsigned long calTimeWas = millis();

  readCompass(x, y, z);

  maxX = minX = x; // Set initial values to current magnetometer readings.
  maxY = minY = y;

  while (cal)
  {

    readCompass(x, y, z);

    if (x > maxX)
      maxX = x;
    if (x < minX)
      minX = x;
    if (y > maxY)
      maxY = y;
    if (y < minY)
      minY = y;

    int secmillis = millis() - calTimeWas;
    int secs = (int)((COMPASS_CAL_TIME - secmillis + 1000) / 1000);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextPadding(tft.textWidth("88"));
    tft.drawNumber((COMPASS_CAL_TIME - secmillis) / 1000, (tft.width() >> 1), 280);

    if (secs == 0)
    {
      offX = (maxX + minX) / 2;
      offY = (maxY + minY) / 2;
      cal = 0;
    }
  }

  tft.setTextSize(1);
  tft.drawCenterString("DONE!", 160, 340, &fonts::DejaVu40);
  tft.drawCenterString("TOUCH TO CONTINUE.", 160, 380, &fonts::DejaVu18);

  while (!tft.getTouch(&touchX, &touchY))
  {
  };

  saveCompassCal(offX,offY);
}