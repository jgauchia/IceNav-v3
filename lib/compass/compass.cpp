/**
 * @file compass.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Compass definition and functions
 * @version 0.2.0_alpha
 * @date 2025-01
 */

#include "compass.hpp"

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
class KalmanFilter
{
public:
  KalmanFilter(float processNoise, float measurementNoise, float estimateError, float initialEstimate)
      : q(processNoise), r(measurementNoise), p(estimateError), x(initialEstimate) {}

  float update(float measurement)
  {
    p = p + q;
    k = p / (p + r);
    x = x + k * (measurement - x);
    x = wrapToPi(x);
    p = (1 - k) * p;
    return x;
  }

private:
  float q; // Process noise covariance
  float r; // Measurement noise covariance
  float p; // Estimate error covariance
  float k; // Kalman gain
  float x; // Value
  float wrapToPi(float angle)
  {
    while (angle < -M_PI)
      angle += 2 * M_PI;
    while (angle > M_PI)
      angle -= 2 * M_PI;
    return angle;
  }
};
KalmanFilter kalmanFilter(0.01, 0.3, 1.0, 0.0);

// Helper function to wrap angle to -pi to pi
float wrapToPi(float angle) {
  while (angle < -M_PI) angle += 2 * M_PI;
  while (angle > M_PI) angle -= 2 * M_PI;
  return angle;
}


/**
 * @brief Compass offset calibration
 * 
 */
float offX = 0.0, offY = 0.0; 

/**
 * @brief Init Compass
 *
 */
void initCompass()
{
  #ifdef HMC5883L
  if (!compass.begin())
    compass.begin();
  compass.setDataRate(HMC5883L_DATARATE_15HZ);
  compass.setSamples(HMC5883L_SAMPLES_1);
  #endif

  #ifdef QMC5883
  if (!compass.begin())
    compass.begin();
  compass.setDataRate(QMC5883_DATARATE_10HZ);
  compass.setSamples(QMC5883_SAMPLES_1);
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

  #ifdef ICENAV_BOARD
    y = y * -1;
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

  // if (headingNoFilter < 0)
  //   headingNoFilter += 2 * M_PI;
  // if (headingNoFilter > 2 * M_PI)
  //   headingNoFilter -= 2 * M_PI;

  headingNoFilter = wrapToPi(headingNoFilter);

  headingSmooth = kalmanFilter.update(headingNoFilter);

  // Convert heading to degrees
  int headingDegrees = (int)(headingSmooth * 180 / M_PI);

  // Wrap heading to stay within 0 to 360 degrees
  if (headingDegrees < 0)
    headingDegrees += 360;
  if (headingDegrees >= 360)
    headingDegrees -= 360;

  return headingDegrees;
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

  cfg.saveFloat(PKEYS::KCOMP_OFFSET_X, offX);
  cfg.saveFloat(PKEYS::KCOMP_OFFSET_Y, offY);
}


