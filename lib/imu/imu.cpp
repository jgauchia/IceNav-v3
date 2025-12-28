/**
 * @file imu.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  IMU definition and functions - Native ESP-IDF driver
 * @version 0.2.4
 * @date 2025-12
 */

#include "imu.hpp"

#ifdef MPU6050

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char* TAG = "IMU";

MPU6050_Driver mpu = MPU6050_Driver();

/**
 * @brief Constructs MPU6050 driver with default configuration.
 *
 * @details Initializes with default I2C address 0x68 and default scale factors
 *          for ±2g accelerometer and ±250°/s gyroscope ranges.
 */
MPU6050_Driver::MPU6050_Driver()
    : i2cAddr(MPU6050_ADDRESS), accelScale(16384.0f), gyroScale(131.0f) {}

/**
 * @brief Reads a single byte from a register.
 * @param reg Register address.
 * @return Register value.
 */
uint8_t MPU6050_Driver::read8(uint8_t reg)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)1);
    return Wire.read();
}

/**
 * @brief Writes a single byte to a register.
 * @param reg Register address.
 * @param value Value to write.
 */
void MPU6050_Driver::write8(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (MSB first).
 * @param reg Starting register address.
 * @return 16-bit signed value.
 */
int16_t MPU6050_Driver::read16(uint8_t reg)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)2);
    int16_t value = Wire.read() << 8;
    value |= Wire.read();
    return value;
}

/**
 * @brief Initializes the MPU6050 sensor.
 *
 * @details Verifies device identity via WHO_AM_I register, wakes sensor from sleep,
 *          and configures default accelerometer (±2g) and gyroscope (±250°/s) ranges.
 *
 * @param addr I2C address (default 0x68).
 * @return true if initialization successful, false otherwise.
 */
bool MPU6050_Driver::begin(uint8_t addr)
{
    i2cAddr = addr;

    uint8_t whoAmI = read8(0x75);
    if (whoAmI != 0x68)
        return false;

    write8(0x6B, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    setAccelRange(0);
    setGyroRange(0);

    return true;
}

/**
 * @brief Sets the accelerometer full-scale range.
 *
 * @details Updates ACCEL_CONFIG register and internal scale factor.
 *
 * @param range 0=±2g, 1=±4g, 2=±8g, 3=±16g.
 */
void MPU6050_Driver::setAccelRange(uint8_t range)
{
    write8(0x1C, range << 3);
    switch (range)
    {
        case 0: accelScale = 16384.0f; break;
        case 1: accelScale = 8192.0f; break;
        case 2: accelScale = 4096.0f; break;
        case 3: accelScale = 2048.0f; break;
    }
}

/**
 * @brief Sets the gyroscope full-scale range.
 *
 * @details Updates GYRO_CONFIG register and internal scale factor.
 *
 * @param range 0=±250°/s, 1=±500°/s, 2=±1000°/s, 3=±2000°/s.
 */
void MPU6050_Driver::setGyroRange(uint8_t range)
{
    write8(0x1B, range << 3);
    switch (range)
    {
        case 0: gyroScale = 131.0f; break;
        case 1: gyroScale = 65.5f; break;
        case 2: gyroScale = 32.8f; break;
        case 3: gyroScale = 16.4f; break;
    }
}

/**
 * @brief Reads accelerometer data.
 *
 * @details Reads raw 16-bit values from ACCEL_XOUT, ACCEL_YOUT, ACCEL_ZOUT registers
 *          and converts to g units using current scale factor.
 *
 * @param x Reference for X-axis acceleration in g.
 * @param y Reference for Y-axis acceleration in g.
 * @param z Reference for Z-axis acceleration in g.
 */
void MPU6050_Driver::getAccel(float &x, float &y, float &z)
{
    x = read16(0x3B) / accelScale;
    y = read16(0x3D) / accelScale;
    z = read16(0x3F) / accelScale;
}

/**
 * @brief Reads gyroscope data.
 *
 * @details Reads raw 16-bit values from GYRO_XOUT, GYRO_YOUT, GYRO_ZOUT registers
 *          and converts to degrees per second using current scale factor.
 *
 * @param x Reference for X-axis angular velocity in °/s.
 * @param y Reference for Y-axis angular velocity in °/s.
 * @param z Reference for Z-axis angular velocity in °/s.
 */
void MPU6050_Driver::getGyro(float &x, float &y, float &z)
{
    x = read16(0x43) / gyroScale;
    y = read16(0x45) / gyroScale;
    z = read16(0x47) / gyroScale;
}

/**
 * @brief Reads internal temperature sensor.
 *
 * @details Reads raw 16-bit value from TEMP_OUT register and applies
 *          conversion formula: Temperature = (raw / 340.0) + 36.53
 *
 * @return Temperature in degrees Celsius.
 */
float MPU6050_Driver::getTemp()
{
    int16_t raw = read16(0x41);
    return (raw / 340.0f) + 36.53f;
}

/**
 * @brief Reads all sensor data in a single I2C transaction.
 *
 * @details Performs burst read of 14 bytes starting from ACCEL_XOUT_H register,
 *          which includes accelerometer, temperature, and gyroscope data.
 *          More efficient than reading each sensor separately.
 *
 * @param ax Reference for X-axis acceleration in g.
 * @param ay Reference for Y-axis acceleration in g.
 * @param az Reference for Z-axis acceleration in g.
 * @param gx Reference for X-axis angular velocity in °/s.
 * @param gy Reference for Y-axis angular velocity in °/s.
 * @param gz Reference for Z-axis angular velocity in °/s.
 * @param temp Reference for temperature in °C.
 */
void MPU6050_Driver::readAll(float &ax, float &ay, float &az,
                              float &gx, float &gy, float &gz, float &temp)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)14);

    ax = (int16_t)(Wire.read() << 8 | Wire.read()) / accelScale;
    ay = (int16_t)(Wire.read() << 8 | Wire.read()) / accelScale;
    az = (int16_t)(Wire.read() << 8 | Wire.read()) / accelScale;
    int16_t rawTemp = Wire.read() << 8 | Wire.read();
    temp = (rawTemp / 340.0f) + 36.53f;
    gx = (int16_t)(Wire.read() << 8 | Wire.read()) / gyroScale;
    gy = (int16_t)(Wire.read() << 8 | Wire.read()) / gyroScale;
    gz = (int16_t)(Wire.read() << 8 | Wire.read()) / gyroScale;
}

/**
 * @brief Initializes the IMU sensor.
 *
 * @details Calls mpu.begin() to initialize hardware and logs result.
 */
void initIMU()
{
    if (!mpu.begin())
    {
        ESP_LOGE(TAG, "Failed to init IMU");
        return;
    }

    ESP_LOGI(TAG, "IMU init OK");
}

#endif
