/**
 * @file bme.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  BME280 Sensor functions - Native ESP-IDF driver
 * @version 0.2.9
 * @date 2026-06
 */

#include "bme.hpp"

#ifdef BME280

#include <algorithm>
#include <cmath>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

BME280_Driver bme = BME280_Driver();

/**
 * @brief Constructs BME280 driver with default configuration.
 */
BME280_Driver::BME280_Driver() :
    I2CDriverBase(BME_ADDRESS),
    dig_T1(0), dig_T2(0), dig_T3(0),
    dig_P1(0), dig_P2(0), dig_P3(0), dig_P4(0), dig_P5(0),
    dig_P6(0), dig_P7(0), dig_P8(0), dig_P9(0),
    dig_H1(0), dig_H2(0), dig_H3(0), dig_H4(0), dig_H5(0), dig_H6(0),
    t_fine(0)
{
}

/**
 * @brief Reads factory calibration coefficients from sensor.
 *
 * @details Loads temperature, pressure, and humidity compensation coefficients
 *          from non-volatile memory. These are used for sensor data compensation.
 */
void BME280_Driver::readCoefficients()
{
    dig_T1 = read16LE(0x88);
    dig_T2 = (int16_t)read16LE(0x8A);
    dig_T3 = (int16_t)read16LE(0x8C);

    dig_P1 = read16LE(0x8E);
    dig_P2 = (int16_t)read16LE(0x90);
    dig_P3 = (int16_t)read16LE(0x92);
    dig_P4 = (int16_t)read16LE(0x94);
    dig_P5 = (int16_t)read16LE(0x96);
    dig_P6 = (int16_t)read16LE(0x98);
    dig_P7 = (int16_t)read16LE(0x9A);
    dig_P8 = (int16_t)read16LE(0x9C);
    dig_P9 = (int16_t)read16LE(0x9E);

    dig_H1 = read8(0xA1);
    dig_H2 = (int16_t)read16LE(0xE1);
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
 * @brief Reads raw 20-bit temperature ADC value from registers 0xFA-0xFC.
 */
int32_t BME280_Driver::readRawT()
{
    uint8_t buf[3];
    i2c.readBytes(i2cAddr, 0xFA, buf, 3);
    int32_t adc = buf[0];
    adc <<= 8;
    adc |= buf[1];
    adc <<= 8;
    adc |= buf[2];
    return adc >> 4;
}

/**
 * @brief Reads raw 20-bit pressure ADC value from registers 0xF7-0xF9.
 */
int32_t BME280_Driver::readRawP()
{
    uint8_t buf[3];
    i2c.readBytes(i2cAddr, 0xF7, buf, 3);
    int32_t adc = buf[0];
    adc <<= 8;
    adc |= buf[1];
    adc <<= 8;
    adc |= buf[2];
    return adc >> 4;
}

/**
 * @brief Reads raw 16-bit humidity ADC value from registers 0xFD-0xFE.
 */
int32_t BME280_Driver::readRawH()
{
    uint8_t buf[2];
    i2c.readBytes(i2cAddr, 0xFD, buf, 2);
    int32_t adc = buf[0];
    adc <<= 8;
    adc |= buf[1];
    return adc;
}

/**
 * @brief Applies Bosch compensation formula to raw temperature ADC.
 *
 * @details Updates t_fine, which must be current before calling compensatePressure
 *          or compensateHumidity.
 *
 * @param adc_T Raw 20-bit temperature ADC value.
 * @return Temperature in degrees Celsius.
 */
float BME280_Driver::compensateTemperature(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return ((t_fine * 5 + 128) >> 8) / 100.0f;
}

/**
 * @brief Applies Bosch compensation formula to raw pressure ADC.
 *
 * @details Requires t_fine to be up to date (call compensateTemperature first).
 *
 * @param adc_P Raw 20-bit pressure ADC value.
 * @return Pressure in Pascals (Pa), or 0 if sensor not ready.
 */
float BME280_Driver::compensatePressure(int32_t adc_P)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0)
        return 0.0f;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (float)p / 256.0f;
}

/**
 * @brief Applies Bosch compensation formula to raw humidity ADC.
 *
 * @details Requires t_fine to be up to date (call compensateTemperature first).
 *
 * @param adc_H Raw 16-bit humidity ADC value.
 * @return Relative humidity in percentage (%RH).
 */
float BME280_Driver::compensateHumidity(int32_t adc_H)
{
    int32_t v = t_fine - (int32_t)76800;

    v = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) -
            (((int32_t)dig_H5) * v)) +
           ((int32_t)16384)) >>
          15) *
         (((((((v * ((int32_t)dig_H6)) >> 10) *
              (((v * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >>
             10) +
            ((int32_t)2097152)) *
               ((int32_t)dig_H2) +
           8192) >>
          14));

    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4);

    v = std::max<int32_t>(v, 0);
    v = std::min<int32_t>(v, 419430400);

    return (float)(v >> 12) / 1024.0f;
}

/**
 * @brief Reads temperature from sensor.
 *
 * @details Reads raw ADC and applies compensation formula. Updates t_fine.
 *
 * @return Temperature in degrees Celsius.
 */
float BME280_Driver::readTemperature()
{
    return compensateTemperature(readRawT());
}

/**
 * @brief Reads atmospheric pressure from sensor.
 *
 * @details Reads temperature first to update t_fine, then reads and compensates pressure.
 *
 * @return Pressure in Pascals (Pa).
 */
float BME280_Driver::readPressure()
{
    compensateTemperature(readRawT());
    return compensatePressure(readRawP());
}

/**
 * @brief Reads relative humidity from sensor.
 *
 * @details Reads temperature first to update t_fine, then reads and compensates humidity.
 *
 * @return Relative humidity in percentage (%RH).
 */
float BME280_Driver::readHumidity()
{
    compensateTemperature(readRawT());
    return compensateHumidity(readRawH());
}

/**
 * @brief Reads all sensor data (T, P, H) in one burst read.
 *
 * @details Performs a single 8-byte I2C transaction to read raw data from
 *          0xF7 to 0xFE, then applies all compensation formulas in order.
 *          This minimises I2C bus traffic and avoids repeated t_fine recalculation.
 *
 * @param temp Reference to store temperature (°C).
 * @param pres Reference to store pressure (Pa).
 * @param humi Reference to store humidity (%RH).
 */
void BME280_Driver::readAll(float &temp, float &pres, float &humi)
{
    uint8_t buffer[8];
    i2c.readBytes(i2cAddr, 0xF7, buffer, 8);

    int32_t adc_P = buffer[0];
    adc_P <<= 8;
    adc_P |= buffer[1];
    adc_P <<= 8;
    adc_P |= buffer[2];
    adc_P >>= 4;

    int32_t adc_T = buffer[3];
    adc_T <<= 8;
    adc_T |= buffer[4];
    adc_T <<= 8;
    adc_T |= buffer[5];
    adc_T >>= 4;

    int32_t adc_H = buffer[6];
    adc_H <<= 8;
    adc_H |= buffer[7];

    temp = compensateTemperature(adc_T);
    pres = compensatePressure(adc_P);
    humi = compensateHumidity(adc_H);
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
float BME280_Driver::readAltitude(float pressure)
{
    return 44330.0f * (1.0f - powf(pressure / 101325.0f, 0.1903f));
}

/**
 * @brief Initializes the BME280 sensor.
 */
void initBME()
{
    bme.begin(BME_ADDRESS);
}

#endif
