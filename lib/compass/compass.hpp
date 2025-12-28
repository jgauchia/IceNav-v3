/**
 * @file compass.hpp
 * @brief Compass definition and functions - Native ESP-IDF drivers
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "tft.hpp"
#include <EasyPreferences.hpp>
#include <Wire.h>

// QMC5883L Register definitions
#define QMC5883L_ADDRESS      0x0D
#define QMC5883L_REG_DATA     0x00
#define QMC5883L_REG_STATUS   0x06
#define QMC5883L_REG_CTRL1    0x09
#define QMC5883L_REG_CTRL2    0x0A
#define QMC5883L_REG_SET_RST  0x0B
#define QMC5883L_REG_CHIP_ID  0x0D

// QMC5883L Configuration values
// Data Rate: 0=10Hz, 1=50Hz, 2=100Hz, 3=200Hz
// Oversampling: 0=512, 1=256, 2=128, 3=64

// HMC5883L Register definitions
#define HMC5883L_ADDRESS      0x1E
#define HMC5883L_REG_CONFIG_A 0x00
#define HMC5883L_REG_CONFIG_B 0x01
#define HMC5883L_REG_MODE     0x02
#define HMC5883L_REG_DATA     0x03
#define HMC5883L_REG_STATUS   0x09
#define HMC5883L_REG_ID_A     0x0A

// HMC5883L Configuration values
// Data Rate: 0=0.75Hz, 1=1.5Hz, 2=3Hz, 3=7.5Hz, 4=15Hz, 5=30Hz, 6=75Hz
// Samples: 0=1, 1=2, 2=4, 3=8

#ifdef HMC5883L
    #define ENABLE_COMPASS
#endif

#ifdef QMC5883
    #define ENABLE_COMPASS
#endif

#ifdef IMU_MPU9250
    #include <MPU9250.h>
    #define ENABLE_COMPASS
    #define ENABLE_IMU
#endif

/**
 * @class QMC5883L_Driver
 * @brief Native ESP-IDF driver for QMC5883L magnetometer.
 */
class QMC5883L_Driver
{
public:
    QMC5883L_Driver();
    bool begin(uint8_t addr = QMC5883L_ADDRESS);
    void setDataRate(uint8_t rate);
    void setSamples(uint8_t samples);
    void readRaw(float &x, float &y, float &z);

private:
    uint8_t i2cAddr;
    uint8_t ctrl1Value;

    uint8_t read8(uint8_t reg);
    void write8(uint8_t reg, uint8_t value);
    int16_t read16(uint8_t reg);
};

/**
 * @class HMC5883L_Driver
 * @brief Native ESP-IDF driver for HMC5883L magnetometer.
 */
class HMC5883L_Driver
{
public:
    HMC5883L_Driver();
    bool begin(uint8_t addr = HMC5883L_ADDRESS);
    void setDataRate(uint8_t rate);
    void setSamples(uint8_t samples);
    void readRaw(float &x, float &y, float &z);

private:
    uint8_t i2cAddr;
    uint8_t configAValue;

    uint8_t read8(uint8_t reg);
    void write8(uint8_t reg, uint8_t value);
    int16_t read16(uint8_t reg);
};

#define COMPASS_CAL_TIME 16000 /**< Compass calibration duration in milliseconds. */

/**
 * @class KalmanFilter
 * @brief Implements a simple 1D Kalman filter for angle estimation.
 */
class KalmanFilter
{
    public:
        KalmanFilter(float processNoise, float measurementNoise, float estimateError, float initialEstimate)
            : q(processNoise), r(measurementNoise), p(estimateError), x(initialEstimate) {}

        /**
        * @brief Updates the state estimate using the Kalman filter algorithm for angular measurements.
        *
        * @details Applies the Kalman filter update step, taking into account the wrapped angular measurement,
        * 		   and updates the internal state and covariance variables accordingly.
        *
        * @param measurement The new angle measurement to incorporate (in radians).
        * @return float The updated state estimate (in radians, wrapped to [-π, π]).
        */
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

        /**
        * @brief Sets the process and measurement noise constants for the Kalman filter.
        *
        * @details Updates the internal parameters for process noise covariance (q) and measurement noise covariance (r).
        *
        * @param processNoise Value for the process noise covariance.
        * @param measurementNoise Value for the measurement noise covariance.
        */
        void setConstants(float processNoise, float measurementNoise)
        {
            q = processNoise;
            r = measurementNoise;
        }

    private:
        float q; /**< Process noise covariance (Q), representing the process variance. */
        float r; /**< Measurement noise covariance (R), representing the sensor variance. */
        float p; /**< Estimate error covariance (P), representing the estimated error. */
        float k; /**< Kalman gain (K), used to update the state estimate. */
        float x; /**< Value (X), the current state estimate. */

        float wrapToPi(float angle)
        {
            while (angle < -M_PI)
                angle += 2 * M_PI;
            while (angle > M_PI)
                angle -= 2 * M_PI;
            return angle;
        }
};

/**
 * @class Compass
 * @brief Provides high-level interface for compass (magnetometer) sensor management and heading calculation.
 */
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
        float declinationAngle;       /**< Magnetic declination angle (in radians or degrees, depending on use). */
        float offX;                   /**< Magnetometer offset for X axis. */
        float offY;                   /**< Magnetometer offset for Y axis. */
        float headingSmooth;          /**< Smoothed heading value. */
        float headingPrevious;        /**< Previous heading value (for smoothing or change detection). */
        float minX;                   /**< Minimum observed value for X axis (for calibration). */
        float maxX;                   /**< Maximum observed value for X axis (for calibration). */
        float minY;                   /**< Minimum observed value for Y axis (for calibration). */
        float maxY;                   /**< Maximum observed value for Y axis (for calibration). */
        bool kalmanFilterEnabled;     /**< True if the Kalman filter is enabled for heading smoothing. */
        KalmanFilter kalmanFilter;    /**< Kalman filter instance used for heading estimation. */
        int previousDegrees;          /**< Previous heading in degrees (integer value). */

        float wrapToPi(float angle);
        float unwrapFromPi(float angle, float previousAngle);
};