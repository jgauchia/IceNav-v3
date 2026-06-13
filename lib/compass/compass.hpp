/**
 * @file compass.hpp
 * @brief Compass definition and functions - Native ESP-IDF drivers
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include <EasyPreferences.hpp>
#include "i2c_espidf.hpp"
#include "i2c_driver_base.hpp"

#ifdef MPU6050
    #include "imu.hpp"
#endif

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

// MPU9250/AK8963 Register definitions
#define MPU9250_ADDRESS         0x68
#define MPU9250_REG_WHO_AM_I    0x75
#define MPU9250_REG_PWR_MGMT1   0x6B
#define MPU9250_REG_INT_PIN     0x37
#define MPU9250_REG_ACCEL_XOUT  0x3B
#define MPU9250_REG_ACCEL_CFG   0x1C

#define AK8963_ADDRESS        0x0C
#define AK8963_REG_WIA        0x00
#define AK8963_REG_ST1        0x02
#define AK8963_REG_DATA       0x03
#define AK8963_REG_CNTL1      0x0A
#define AK8963_REG_CNTL2      0x0B
#define AK8963_REG_ASAX       0x10

// AK8963 Configuration values
// Mode: 0=PowerDown, 1=Single, 2=Continuous8Hz, 6=Continuous100Hz
// Resolution: 0=14bit, 1=16bit (bit 4)

#ifdef HMC5883L
    #define ENABLE_COMPASS
#endif

#ifdef QMC5883
    #define ENABLE_COMPASS
#endif

#ifdef IMU_MPU9250
    #define ENABLE_COMPASS
    #define ENABLE_IMU
#endif

#ifdef ENABLE_IMU
    #ifndef IMU_ACCEL_X_SIGN
        #define IMU_ACCEL_X_SIGN 1
    #endif
    #ifndef IMU_ACCEL_Y_SIGN
        #define IMU_ACCEL_Y_SIGN 1
    #endif
    #ifndef IMU_ACCEL_Z_SIGN
        #define IMU_ACCEL_Z_SIGN 1
    #endif
#endif

/**
 * @class QMC5883L_Driver
 * @brief Native ESP-IDF driver for QMC5883L magnetometer.
 */
class QMC5883L_Driver : public I2CDriverBase
{
public:
    QMC5883L_Driver();
    bool begin(uint8_t addr = QMC5883L_ADDRESS);
    bool setDataRate(uint8_t rate);
    bool setSamples(uint8_t samples);
    bool readRaw(float &x, float &y, float &z);

private:
    uint8_t ctrl1Value;
};

/**
 * @class HMC5883L_Driver
 * @brief Native ESP-IDF driver for HMC5883L magnetometer.
 */
class HMC5883L_Driver : public I2CDriverBase
{
public:
    HMC5883L_Driver();
    bool begin(uint8_t addr = HMC5883L_ADDRESS);
    void setDataRate(uint8_t rate);
    void setSamples(uint8_t samples);
    bool readRaw(float &x, float &y, float &z);

private:
    uint8_t configAValue;
};

/**
 * @class MPU9250_Driver
 * @brief Native ESP-IDF driver for MPU9250 with AK8963 magnetometer.
 */
class MPU9250_Driver
{
public:
    MPU9250_Driver();
    bool begin(uint8_t addr = MPU9250_ADDRESS);
    void readSensor();
    void readAccel(float &ax, float &ay, float &az);
    float getMagX_uT();
    float getMagY_uT();
    float getMagZ_uT();

private:
    uint8_t mpuAddr;
    uint8_t akAddr;
    float magX;
    float magY;
    float magZ;
    float asaX;
    float asaY;
    float asaZ;
    float accelScale;

    uint8_t read8(uint8_t addr, uint8_t reg);
    void write8(uint8_t addr, uint8_t reg, uint8_t value);
    int16_t read16LE(uint8_t addr, uint8_t reg);
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
            : q(processNoise), r(measurementNoise), p(estimateError), x(initialEstimate) 
        {
            k = 0.0f;
        }

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
        bool read(float &x, float &y, float &z);
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
