/**
 * @file compass.hpp
 * @brief Compass definition and functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef COMPASS_HPP
#define COMPASS_HPP

#include "tft.hpp"
#include <EasyPreferences.hpp>

#ifdef HMC5883L
#include <DFRobot_QMC5883.h>
#define ENABLE_COMPASS
#endif

#ifdef QMC5883
#include <DFRobot_QMC5883.h>
#define ENABLE_COMPASS
#endif

#ifdef IMU_MPU9250
#include <MPU9250.h>
#define ENABLE_COMPASS
#define ENABLE_IMU ;
#endif

#define COMPASS_CAL_TIME 16000

class KalmanFilter
{
public:
  KalmanFilter(float processNoise, float measurementNoise, float estimateError, float initialEstimate)
    : q(processNoise), r(measurementNoise), p(estimateError), x(initialEstimate) {}

  float update(float measurement)
  {
    measurement = wrapToPi(measurement);
    float y = measurement - x;
    y = wrapToPi(y);

    p = p + q;
    k = p / (p + r);
    x = x + k * y;

    x = wrapToPi(x);

    p = (1 - k) * p;
    return x;
  }

  void setConstants(float processNoise, float measurementNoise)
  {
    q = processNoise;
    r = measurementNoise;
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

class Compass
{
public:
  Compass();
  void init();
  void read(float &x, float &y, float &z);
  int getHeading();
  bool isUpdated();
  void calibrate();
  void setDeclinationAngle(float angle);
  void setOffsets(float offsetX, float offsetY);
  void enableKalmanFilter(bool enabled);
  void setKalmanFilterConst(float processNoise, float measureNoise);

private:
  float declinationAngle;
  float offX;
  float offY;
  float headingSmooth;
  float headingPrevious;
  float minX;
  float maxX;
  float minY;
  float maxY;
  bool kalmanFilterEnabled;
  KalmanFilter kalmanFilter;
  int previousDegrees;

  float wrapToPi(float angle);
  float unwrapFromPi(float angle, float previousAngle);
};

#endif 
