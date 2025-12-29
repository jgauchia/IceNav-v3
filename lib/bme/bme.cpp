/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions - Native ESP-IDF driver
 * @version 0.2.4
 * @date 2025-12
 */

#include "bme.hpp"

#ifdef BME280

#include <cmath>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

BME280_Driver bme = BME280_Driver();
uint8_t tempValue = 0;
uint8_t tempOld = 0;

/**
 * @brief Constructs BME280 driver with default configuration.
 *
 * @details Initializes with default I2C address and resets t_fine compensation value.
 */
BME280_Driver::BME280_Driver() : i2cAddr(BME_ADDRESS), t_fine(0) {}

/**
 * @brief Reads a single byte from a register.
 * @param reg Register address.
 * @return Register value.
 */
uint8_t BME280_Driver::read8(uint8_t reg)
{
    return i2c.read8(i2cAddr, reg);
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (LSB first).
 * @param reg Starting register address.
 * @return 16-bit unsigned value in little-endian format.
 */
uint16_t BME280_Driver::read16_LE(uint8_t reg)
{
    uint8_t buffer[2];
    i2c.readBytes(i2cAddr, reg, buffer, 2);
    return (uint16_t)((buffer[1] << 8) | buffer[0]);
}

/**
 * @brief Reads a signed 16-bit value from two consecutive registers (LSB first).
 * @param reg Starting register address.
 * @return 16-bit signed value in little-endian format.
 */
int16_t BME280_Driver::readS16_LE(uint8_t reg)
{
    return (int16_t)read16_LE(reg);
}

/**
 * @brief Writes a single byte to a register.
 * @param reg Register address.
 * @param value Value to write.
 */
void BME280_Driver::write8(uint8_t reg, uint8_t value)
{
    i2c.write8(i2cAddr, reg, value);
}

/**
 * @brief Reads factory calibration coefficients from sensor.
 *
 * @details Loads temperature, pressure, and humidity compensation coefficients
 *          from non-volatile memory. These are used for sensor data compensation.
 */
void BME280_Driver::readCoefficients()
{
    dig_T1 = read16_LE(0x88);
    dig_T2 = readS16_LE(0x8A);
    dig_T3 = readS16_LE(0x8C);

    dig_P1 = read16_LE(0x8E);
    dig_P2 = readS16_LE(0x90);
    dig_P3 = readS16_LE(0x92);
    dig_P4 = readS16_LE(0x94);
    dig_P5 = readS16_LE(0x96);
    dig_P6 = readS16_LE(0x98);
    dig_P7 = readS16_LE(0x9A);
    dig_P8 = readS16_LE(0x9C);
    dig_P9 = readS16_LE(0x9E);

    dig_H1 = read8(0xA1);
    dig_H2 = readS16_LE(0xE1);
    dig_H3 = read8(0xE3);
    dig_H4 = ((int16_t)read8(0xE4) << 4) | (read8(0xE5) & 0x0F);
    dig_H5 = ((int16_t)read8(0xE6) << 4) | (read8(0xE5) >> 4);
    dig_H6 = (int8_t)read8(0xE7);
}

/**
 * @brief Initializes the BME280 sensor.
 *
 * @details Verifies device identity via chip ID register, performs soft reset,
 *          reads calibration coefficients, and configures normal mode with
 *          1x oversampling for all measurements and 1000ms standby time.
 *
 * @param addr I2C address (default 0x76 or 0x77).
 * @return true if initialization successful, false otherwise.
 */
bool BME280_Driver::begin(uint8_t addr)
{
    i2cAddr = addr;

    uint8_t chipId = read8(0xD0);
    if (chipId != 0x60)
        return false;

    write8(0xE0, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));

    readCoefficients();

    write8(0xF2, 0x01);
    write8(0xF4, 0x27);
    write8(0xF5, 0xA0);

    return true;
}

/**
 * @brief Reads temperature from sensor.
 *
 * @details Reads raw 20-bit ADC value from temperature registers and applies
 *          compensation formula using factory calibration coefficients.
 *          Updates internal t_fine value used by pressure and humidity calculations.
 *
 * @return Temperature in degrees Celsius.
 */
float BME280_Driver::readTemperature()
{
    uint8_t buffer[3];
    i2c.readBytes(i2cAddr, 0xFA, buffer, 3);

    int32_t adc_T = buffer[0];
    adc_T <<= 8;
    adc_T |= buffer[1];
    adc_T <<= 8;
    adc_T |= buffer[2];
    adc_T >>= 4;

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

/**
 * @brief Reads atmospheric pressure from sensor.
 *
 * @details Reads raw 20-bit ADC value from pressure registers and applies
 *          compensation formula using factory calibration coefficients.
 *          Internally calls readTemperature() to update t_fine value.
 *
 * @return Pressure in Pascals (Pa).
 */
float BME280_Driver::readPressure()
{
    readTemperature();

    uint8_t buffer[3];
    i2c.readBytes(i2cAddr, 0xF7, buffer, 3);

    int32_t adc_P = buffer[0];
    adc_P <<= 8;
    adc_P |= buffer[1];
    adc_P <<= 8;
    adc_P |= buffer[2];
    adc_P >>= 4;

    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0)
        return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (float)p / 256.0f;
}

/**
 * @brief Reads relative humidity from sensor.
 *
 * @details Reads raw 16-bit ADC value from humidity registers and applies
 *          compensation formula using factory calibration coefficients.
 *          Internally calls readTemperature() to update t_fine value.
 *
 * @return Relative humidity in percentage (%RH).
 */
float BME280_Driver::readHumidity()
{
    readTemperature();

    uint8_t buffer[2];
    i2c.readBytes(i2cAddr, 0xFD, buffer, 2);

    int32_t adc_H = buffer[0];
    adc_H <<= 8;
    adc_H |= buffer[1];

    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));

    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) -
                   (((int32_t)dig_H5) * v_x1_u32r)) +
                  ((int32_t)16384)) >>
                 15) *
                (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
                     (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >>
                    10) +
                   ((int32_t)2097152)) *
                      ((int32_t)dig_H2) +
                  8192) >>
                 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)dig_H1)) >>
                              4));

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

    float h = (v_x1_u32r >> 12);
    return h / 1024.0f;
}

/**
 * @brief Calculates altitude based on atmospheric pressure.
 *
 * @details Uses barometric formula to estimate altitude from current pressure
 *          reading compared to sea level reference pressure.
 *
 * @param seaLevelPressure Reference sea level pressure in Pascals (default 101325 Pa).
 * @return Estimated altitude in meters.
 */
float BME280_Driver::readAltitude(float seaLevelPressure)
{
    float pressure = readPressure();
    return 44330.0f * (1.0f - powf(pressure / seaLevelPressure, 0.1903f));
}

/**
 * @brief Initializes the BME280 sensor.
 *
 * @details Calls bme.begin() to initialize hardware with default I2C address.
 */
void initBME()
{
    bme.begin(BME_ADDRESS);
}

#endif
