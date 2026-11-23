/**
 * @file dps310.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  DPS310 Sensor functions - Native ESP-IDF driver
 * @version 0.3.3
 * @date 2026-10
 */

#include "dps310.hpp"

#ifdef DPS310

#include <cmath>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// DPS310 register map (Infineon datasheet v1.02)
#define DPS310_REG_PRS_B2   0x00
#define DPS310_REG_TMP_B2   0x03
#define DPS310_REG_PRS_CFG  0x06
#define DPS310_REG_TMP_CFG  0x07
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_CFG      0x09
#define DPS310_REG_RESET    0x0C
#define DPS310_REG_PRODREV  0x0D
#define DPS310_REG_COEF     0x10
#define DPS310_REG_COEF_SRCE 0x28

// MEAS_CFG status bits
#define DPS310_PSR_RDY       0x10
#define DPS310_TMP_RDY       0x20
#define DPS310_SENSOR_RDY    0x40
#define DPS310_COEF_RDY      0x80

// TMP_EXT bit of TMP_CFG, must mirror COEF_SRCE bit 7
#define DPS310_TMP_EXT       0x80

// Soft reset command (Linux kernel dps310.c: DPS310_RESET_MAGIC = 0x09)
#define DPS310_RESET_MAGIC  0x09
#define DPS310_PROD_ID      0x10   // Product ID 0x1 in the high nibble of PRODREV

// Secret-register workaround for the known production defect that leaves the
// internal temperature element stuck at ~90..100 °C (Infineon DpsClass::correctTemp).
// IMPORTANT: like the Linux kernel (dps310_temp_workaround), only apply it when
// undocumented register 0x32 bit 1 is clear — forcing the 0x02 gain on chips
// that don't need it corrupts the raw temperature scale.
#define DPS310_REG_UNLOCK   0x0E
#define DPS310_REG_UNLKKEY1 0xA5
#define DPS310_REG_SECKEY   0x0F
#define DPS310_REG_UNLKKEY2 0x96
#define DPS310_REG_TMPFIX   0x62
#define DPS310_TMPFIX_VALUE 0x02
#define DPS310_REG_TMPFIX_STATE  0x32  // undocumented; bit 1 set = chip OK, no quirk needed
#define DPS310_TMPFIX_OK_MASK    0x02

#define DPS310_NUM_COEF    18

// Configuration: continuous pressure + temperature, 1 Hz rate, 8x oversampling
#define DPS310_MEAS_CFG_CONT_BOTH 0x07
#define DPS310_CMD_TEMP           0x02   // MEAS_CFG: command mode, one temperature measurement
//
// TMP_CFG/PRS_CFG bit fields (datasheet Table 8): bits[6..4] = measurement
// rate, bits[3..0] = oversampling rate (PRC). Getting these two swapped
// rescales the raw results by the corresponding scale-factor ratio.
#define DPS310_RATE_1HZ           0x00   // rate index 0 -> 1 Hz
#define DPS310_OVERSAMPLING_8     0x03   // PRC index 3 -> 8x oversampling

// kP/kT scale factor for 8x oversampling (datasheet Table 9); no P/T shift needed
#define DPS310_SCALE_8X  7864320.0f

DPS310_Driver dps310 = DPS310_Driver();

/**
 * @brief Sign-extends a raw value read from the device.
 *
 * @param raw Raw unsigned value.
 * @param bits Number of significant bits in raw.
 * @return 32-bit signed value.
 */
static int32_t signExtend(uint32_t raw, uint8_t bits)
{
    uint32_t mask = (1u << bits) - 1u;
    raw &= mask;
    if (raw & (1u << (bits - 1)))
        raw -= (1u << bits);
    return static_cast<int32_t>(raw);
}

/**
 * @brief Constructs DPS310 driver with default configuration.
 */
DPS310_Driver::DPS310_Driver() :
    I2CDriverBase(DPS310_ADDRESS),
    c0(0), c1(0), c00(0), c10(0), c01(0), c11(0), c20(0), c21(0), c30(0),
    kP(DPS310_SCALE_8X), kT(DPS310_SCALE_8X),
    detected(false)
{
}

/**
 * @brief Reads factory calibration coefficients from sensor.
 *
 * @details Loads the 20 coefficients from non-volatile memory (registers 0x10
 *          to 0x21) into signed 12/20/16-bit calibrated values, as described
 *          in datasheet section 8.11.
 */
void DPS310_Driver::readCoefficients()
{
    uint8_t coef[DPS310_NUM_COEF];
    readBytes(DPS310_REG_COEF, coef, DPS310_NUM_COEF);

    c0  = signExtend(((uint32_t)coef[0] << 4) | (coef[1] >> 4), 12);
    c1  = signExtend(((uint32_t)(coef[1] & 0x0F) << 8) | coef[2], 12);
    c00 = signExtend(((uint32_t)coef[3] << 12) | ((uint32_t)coef[4] << 4) | (coef[5] >> 4), 20);
    c10 = signExtend(((uint32_t)(coef[5] & 0x0F) << 16) | ((uint32_t)coef[6] << 8) | coef[7], 20);
    c01 = signExtend(((uint32_t)coef[8] << 8) | coef[9], 16);
    c11 = signExtend(((uint32_t)coef[10] << 8) | coef[11], 16);
    c20 = signExtend(((uint32_t)coef[12] << 8) | coef[13], 16);
    c21 = signExtend(((uint32_t)coef[14] << 8) | coef[15], 16);
    c30 = signExtend(((uint32_t)coef[16] << 8) | coef[17], 16);
}

/**
 * @brief Brings up the sensor for both begin() and beginShared().
 *
 * @details Soft resets the device, waits for the sensor and its coefficients
 *          to become ready, verifies the product ID, reads the calibration
 *          coefficients and switches to continuous background measurement.
 *
 * @return true if initialization successful, false otherwise.
 */
bool DPS310_Driver::init()
{
    write8(DPS310_REG_RESET, DPS310_RESET_MAGIC);
    vTaskDelay(pdMS_TO_TICKS(10));

    // The device needs ~12ms for SENSOR_RDY and ~40ms for COEF_RDY (datasheet 4.3)
    bool ready = false;
    for (uint8_t i = 0; i < 20; ++i)
    {
        uint8_t measCfg = read8(DPS310_REG_MEAS_CFG);
        if ((measCfg & DPS310_SENSOR_RDY) && (measCfg & DPS310_COEF_RDY))
        {
            ready = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (!ready)
        return false;

    uint8_t prodRev = read8(DPS310_REG_PRODREV);
    if ((prodRev & 0xF0) != DPS310_PROD_ID)
        return false;

    // The temperature source used to generate the coefficients must be
    // mirrored in the TMP_CFG.TMP_EXT bit (see ESPHome/Linux kernel drivers)
    uint8_t coefSrce = read8(DPS310_REG_COEF_SRCE);

    readCoefficients();
    kP = DPS310_SCALE_8X;
    kT = DPS310_SCALE_8X;

    write8(DPS310_REG_PRS_CFG, (DPS310_RATE_1HZ << 4) | DPS310_OVERSAMPLING_8);
    bool tempExt = coefSrce & DPS310_TMP_EXT;   // Coefficient source strap as first guess
    write8(DPS310_REG_TMP_CFG, (DPS310_RATE_1HZ << 4) | DPS310_OVERSAMPLING_8 | (tempExt ? DPS310_TMP_EXT : 0));
    write8(DPS310_REG_CFG,     0x00);

    // Production-defect workaround, gated exactly like the Linux kernel
    // (dps310_temp_workaround): undocumented register 0x32 bit 1 set means the
    // chip is OK and the secret gain correction MUST NOT be applied — writing
    // it to a healthy chip corrupts the raw temperature scale.
    uint8_t reg32 = read8(DPS310_REG_TMPFIX_STATE);
    bool needsQuirk = !(reg32 & DPS310_TMPFIX_OK_MASK);    // The forced measurement below doubles as a temperature-source sanity
    // check: a wrong or self-heated element reads way out of the sensor's
    // operating range (-40..85 °C), so fall back to the other element
    // (TMP_CFG.TMP_EXT) and keep whichever gives a plausible value.
    int32_t rawT = 0;
    forceTemp(rawT);
    float testTemp = (float)c0 * 0.5f + (float)c1 * ((float)rawT / kT);

    if (needsQuirk)
    {
        // Infineon DpsClass::correctTemp: unlock the secret registers, fix the
        // internal temperature element gain, re-lock.
        write8(DPS310_REG_UNLOCK, DPS310_REG_UNLKKEY1);
        write8(DPS310_REG_SECKEY, DPS310_REG_UNLKKEY2);
        write8(DPS310_REG_TMPFIX, DPS310_TMPFIX_VALUE);
        write8(DPS310_REG_UNLOCK, 0x00);
        write8(DPS310_REG_SECKEY, 0x00);

        forceTemp(rawT);
        testTemp = (float)c0 * 0.5f + (float)c1 * ((float)rawT / kT);
        printf("DPS310 temperature gain workaround applied, first reading %.1f °C\n", testTemp);
    }

    if (testTemp < -40.0f || testTemp > 85.0f)
    {
        tempExt = !tempExt;
        write8(DPS310_REG_TMP_CFG, (DPS310_RATE_1HZ << 4) | DPS310_OVERSAMPLING_8 | (tempExt ? DPS310_TMP_EXT : 0));
        int32_t rawT2 = 0;
        if (forceTemp(rawT2))
        {
            rawT = rawT2;
            testTemp = (float)c0 * 0.5f + (float)c1 * ((float)rawT / kT);
        }
    }
    if (testTemp < -40.0f || testTemp > 85.0f)
    {
        printf("DPS310 WARNING: implausible temperature on both elements (%.1f °C), keeping coefficient source\n", testTemp);
    }
    else
    {
        printf("DPS310 temperature element: %s, first reading %.1f °C\n", tempExt ? "MEMS (external)" : "ASIC (internal)", testTemp);
    }

    write8(DPS310_REG_MEAS_CFG, DPS310_MEAS_CFG_CONT_BOTH);

    return true;
}

/**
 * @brief Triggers one temperature measurement in command mode.
 *
 * @details Used both to refresh the temperature value cached internally for
 *          pressure compensation, and to probe the temperature source at init.
 *          Blocking for up to ~90 ms while the result is generated.
 *
 * @param rawT Receives the raw 24-bit temperature result.
 * @return true if the measurement completed and a raw value was read.
 */
bool DPS310_Driver::forceTemp(int32_t &rawT)
{
    // Kernel note: MEAS_CFG doesn't update correctly unless first written
    // with 0 (standby), and a previous measurement leaves TMP_RDY set, so
    // go through standby to guarantee a fresh result is awaited.
    write8(DPS310_REG_MEAS_CFG, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    write8(DPS310_REG_MEAS_CFG, DPS310_CMD_TEMP);
    // 8x oversampling measurement takes ~15 ms (datasheet Table 16)
    vTaskDelay(pdMS_TO_TICKS(20));
    for (uint8_t i = 0; i < 18; ++i)
    {
        if (read8(DPS310_REG_MEAS_CFG) & DPS310_TMP_RDY)
        {
            uint8_t buf[3];
            readBytes(DPS310_REG_TMP_B2, buf, 3);
            rawT = signExtend(((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

/**
 * @brief Probes a single I2C address and initializes the sensor there.
 *
 * @param addr Candidate I2C address.
 * @return true if initialization successful, false otherwise.
 */
bool DPS310_Driver::probe(uint8_t addr)
{
    i2cAddr = addr;
    detected = init();
    // printf instead of ESP_LOGI: the project log level is ERROR-only,
    // info diagnostics would never reach the serial console
    if (detected)
        printf("DPS310 detected at 0x%02X\n", addr);
    return detected;
}

/**
 * @brief Initializes the DPS310 sensor.
 *
 * @details Auto-probes the two possible I2C addresses (0x77 with SDO high,
 *          0x76 with SDO low) unless one is passed explicitly.
 *
 * @param addr I2C address, or 0 to auto-probe both addresses.
 * @return true if initialization successful, false otherwise.
 */
bool DPS310_Driver::begin(uint8_t addr)
{
    if (addr)
        return probe(addr);
    if (probe(DPS310_ADDRESS))
        return true;
    return probe(DPS310_ADDR_ALT);
}

#ifdef TOUCH_CAPACITIVE
/**
 * @brief Initializes the DPS310 sensor over an I2C bus already owned by LovyanGFX.
 *
 * @details For boards where the touch panel is I2C (TOUCH_CAPACITIVE), shares
 *          the bus already brought up for the touch controller instead of
 *          opening a second bus — the new i2c_master driver forbids a second
 *          owner of the same port. Auto-probes both I2C addresses like begin().
 *
 * @param i2cPort I2C port already initialized by LovyanGFX (touch bus).
 * @param addr    I2C address, or 0 to auto-probe both addresses.
 * @return true if initialization successful, false otherwise.
 */
bool DPS310_Driver::beginShared(int i2cPort, uint8_t addr)
{
    I2CDriverBase::beginShared(i2cPort);
    if (addr)
        return probe(addr);
    if (probe(DPS310_ADDRESS))
        return true;
    return probe(DPS310_ADDR_ALT);
}
#endif

/**
 * @brief Applies Infineon compensation formulas to raw measurements.
 *
 * @details Datasheet sections 4.9.1 (temperature) and 4.9.2 (pressure),
 *          with raw results scaled by the compensation scale factors kT and
 *          kP chosen at init().
 *
 * @param temp Reference to store temperature (°C).
 * @param pres Reference to store pressure (Pa).
 */
void DPS310_Driver::compensate(float &temp, float &pres)
{
    uint8_t buf[6];
    // Registers 0x00..0x05 hold pressure B2/B1/B0 and temperature B2/B1/B0
    readBytes(DPS310_REG_PRS_B2, buf, 6);

    int32_t rawP = signExtend(((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);
    int32_t rawT = signExtend(((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5], 24);

    float pRaw = (float)rawP / kP;
    float tRaw = (float)rawT / kT;

    temp = (float)c0 * 0.5f + (float)c1 * tRaw;
    pres = (float)c00 + pRaw * ((float)c10 + pRaw * ((float)c20 + pRaw * (float)c30))
         + tRaw * (float)c01
         + tRaw * pRaw * ((float)c11 + pRaw * (float)c21);
}

/**
 * @brief Calculates altitude based on atmospheric pressure.
 *
 * @details Uses barometric formula to estimate altitude from current pressure
 *          reading compared to sea level reference pressure.
 *
 * @param pressure Measured pressure in Pascals (Pa).
 * @return Estimated altitude in meters.
 */
float DPS310_Driver::readAltitude(float pressure)
{
    return 44330.0f * (1.0f - powf(pressure / 101325.0f, 0.1903f));
}

/**
 * @brief Reads all sensor data (T, P) in one burst read.
 *
 * @details Performs a single 6-byte I2C transaction covering both raw result
 *          registers, then applies the compensation formulas. The DPS310 has
 *          no humidity element, so humi is always 0.
 *
 * @param temp Reference to store temperature (°C).
 * @param pres Reference to store pressure (Pa).
 * @param humi Reference to store humidity (%RH), always 0 on this sensor.
 */
void DPS310_Driver::readAll(float &temp, float &pres, float &humi)
{
    compensate(temp, pres);
    humi = 0.0f;
}

/**
 * @brief Initializes the DPS310 sensor.
 */
void initDPS310()
{
    dps310.begin();
}

#endif
