/**
 * @file compass.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Compass definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#include "compass.hpp"

int mapHeading = 0;

#ifdef HMC5883L
DFRobot_QMC5883 compass = DFRobot_QMC5883(&Wire,HMC5883L_ADDRESS);
#endif

#ifdef QMC5883
DFRobot_QMC5883 compass = DFRobot_QMC5883(&Wire,QMC5883_ADDRESS);
#endif

#ifdef IMU_MPU9250
MPU9250 IMU = MPU9250(Wire, 0x68);
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
 * @brief Compass Heading Angle and Smooth factors
 *
 */
int heading = 0;
#define SMOOTH_FACTOR 0.40
#define SMOOTH_PREVIOUS_FACTOR 0.60

/**
 * @brief Compass offset calibration
 * 
 */
float offX = 0.0, offY = 0.0; 

/**
:wa
* @brief Init Compass
 *
 */
void initCompass()
{
  #ifdef HMC5883L
  if (!compass.begin())
    compass.begin();
  compass.setDataRate(HMC5883L_DATARATE_15HZ);
  compass.setSamples(HMC5883L_SAMPLES_2);
  #endif

  #ifdef QMC5883
  if (!compass.begin())
    compass.begin();
  compass.setDataRate(QMC5883_DATARATE_10HZ);
  compass.setSamples(QMC5883_SAMPLES_2);
  #endif

  #ifdef IMU_MPU9250
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
void readCompass(float &x, float &y, float &z)
{
  #ifdef HMC5883L
  sVector_t mag = compass.readRaw();
  y = mag.YAxis;
  x = mag.XAxis;
  z = mag.ZAxis;
  #endif

  #ifdef QMC5883 
  sVector_t mag = compass.readRaw();
  y = mag.YAxis;
  x = mag.XAxis;
  z = mag.ZAxis;
  #endif

   #ifdef IMU_MPU9250
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
void compassCalibrate()
{
  bool cal = 1;
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;
  uint16_t touchX, touchY;

  static const lgfx::v1::GFXfont* fontSmall;
  static const lgfx::v1::GFXfont* fontLarge;

  #ifdef LARGE_SCREEN
    fontSmall = &fonts::DejaVu18;
    fontLarge = &fonts::DejaVu40;
    static const float scale = 1.0f;
  #else
    fontSmall = &fonts::DejaVu12;
    fontLarge = &fonts::DejaVu24;
    static const float scale = 0.75f;
  #endif

  tft.drawCenterString("ROTATE THE DEVICE", tft.width() >> 1, 10 * scale, fontSmall);
  tft.drawPngFile(PSTR("/spiffs/turn.png"), (tft.width() / 2) - 50, 60 * scale);
  tft.drawCenterString("TOUCH TO START", tft.width() >> 1, 200 * scale, fontSmall);
  tft.drawCenterString("COMPASS CALIBRATION", tft.width() >> 1, 230 * scale, fontSmall);

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
    tft.drawNumber((COMPASS_CAL_TIME - secmillis) / 1000, (tft.width() >> 1), 280 * scale);

    if (secs == 0)
    {
      offX = (maxX + minX) / 2;
      offY = (maxY + minY) / 2;
      cal = 0;
    }
  }

  tft.setTextSize(1);
  tft.drawCenterString("DONE!", tft.width() >> 1, 340 * scale, fontLarge);
  tft.drawCenterString("TOUCH TO CONTINUE.", tft.width() >> 1, 380 * scale, fontSmall);

  while (!tft.getTouch(&touchX, &touchY))
  {
  };

  saveCompassCal(offX,offY);
}


