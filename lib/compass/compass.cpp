/**
 * @file compass.cpp
 * @brief Compass definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

#include "compass.hpp"

static const char* TAG PROGMEM = "Compass";

#ifdef HMC5883L
DFRobot_QMC5883 comp = DFRobot_QMC5883(&Wire, HMC5883L_ADDRESS);
#endif

#ifdef QMC5883
DFRobot_QMC5883 comp = DFRobot_QMC5883(&Wire, QMC5883_ADDRESS);
#endif

#ifdef IMU_MPU9250
MPU9250 IMU = MPU9250(Wire, 0x68);
#endif

/**
 * @brief Compass Class constructor with default values
 *
 */
Compass::Compass()
    : declinationAngle(0.22), offX(0.0), offY(0.0),
      headingSmooth(0.0), headingPrevious(0.0),
      minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
      kalmanFilterEnabled(true),
      kalmanFilter(0.01, 0.1, 1.0, 0.0)
{
}

/**
 * @brief Compass Init
 *
 */
void Compass::init()
{
#ifdef HMC5883L
  if (!comp.begin())
    comp.begin();
  comp.setDataRate(HMC5883L_DATARATE_15HZ);
  comp.setSamples(HMC5883L_SAMPLES_1);
#endif

#ifdef QMC5883
  if (!comp.begin())
    comp.begin();
  comp.setDataRate(QMC5883_DATARATE_10HZ);
  comp.setSamples(QMC5883_SAMPLES_1);
#endif

#ifdef IMU_MPU9250
  int status = IMU.begin();
  if (status < 0)
  {
    ESP_LOGE(TAG, "IMU initialization unsuccessful");
    ESP_LOGE(TAG, "Check IMU wiring or try cycling power");
    ESP_LOGE(TAG, "Status: %i", status);
  }
#endif
}

/**
 * @brief Configure ADC Channel for battery reading
 *
 * @param x 
 * @param y 
 * @param z 
 */
void Compass::read(float &x, float &y, float &z)
{
#ifdef HMC5883L
  sVector_t mag = comp.readRaw();
  y = mag.YAxis;
  x = mag.XAxis;
  z = mag.ZAxis;
#endif

#ifdef QMC5883
  sVector_t mag = comp.readRaw();
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
int Compass::getHeading()
{
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;

  read(x, y, z);

  float headingNoFilter = atan2(y - offY, x - offX);
  headingNoFilter += declinationAngle;

  headingNoFilter = wrapToPi(headingNoFilter);

  if (kalmanFilterEnabled)
  {
    headingNoFilter = unwrapFromPi(headingNoFilter, headingPrevious);
    headingSmooth = kalmanFilter.update(headingNoFilter);
  }
  else
    headingSmooth = headingNoFilter;

    headingPrevious = headingNoFilter;

  float headingDegrees = (int)(headingSmooth * 180 / M_PI);

  if (headingDegrees < 0)
    headingDegrees += 360;

  return static_cast<int>(headingDegrees);
}

/**
 * @brief Check compass heading is update
 *
 * @return true/flase
 */
bool Compass::isUpdated()
{
  int currentDegrees = getHeading();
  if (currentDegrees != previousDegrees)
  {
    previousDegrees = currentDegrees;
    return true;
  }
  return false;
}

/**
 * @brief Compass calibration
 *
 */
void Compass::calibrate()
{
  bool cal = 1;
  float y = 0.0;
  float x = 0.0;
  float z = 0.0;
  uint16_t touchX, touchY;

  static const lgfx::v1::GFXfont *fontSmall;
  static const lgfx::v1::GFXfont *fontLarge;

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

  read(x, y, z);

  maxX = minX = x;
  maxY = minY = y;

  while (cal)
  {
    read(x, y, z);

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

/**
 * @brief Set compass declination angle
 * 
 * @param angle -> Declination angle
 */
void Compass::setDeclinationAngle(float angle)
{
  declinationAngle = angle;
}

/**
 * @brief Set compass calibration offsets
 * 
 * @param offsetX
 * @param offsetY
 */
void Compass::setOffsets(float offsetX, float offsetY)
{
  offX = offsetX;
  offY = offsetY;
}

/**
 * @brief Enable compass Kalman filter
 * 
 * @param enable -> true/false
 */
void Compass::enableKalmanFilter(bool enabled)
{
  kalmanFilterEnabled = enabled;
}

/**
 * @brief Set Kalman filter constants
 * 
 * @param processNoise -> 0-1
 * @param measureNoise -> 0-1
 */
void Compass::setKalmanFilterConst(float processNoise, float measureNoise)
{
  kalmanFilter.setConstants(processNoise, measureNoise);
}

/**
 * @brief Helper function to wrap angle to -pi to pi
 * 
 * @param angle
 * @return wrap angle
 */
float Compass::wrapToPi(float angle)
{
  while (angle < -M_PI)
    angle += 2 * M_PI;
  while (angle > M_PI)
    angle -= 2 * M_PI;
  return angle;
}

/**
 * @brief Helper function to unwrap angle to -pi to pi
 * 
 * @param angle
 * @param previousAngle
 * @return unrap angle
 */
float Compass::unwrapFromPi(float angle, float previousAngle)
{
  float delta = angle - previousAngle;
  if (delta > M_PI)
    angle -= 2 * M_PI;
  else if (delta < -M_PI)
    angle += 2 * M_PI;
  return angle;
}
