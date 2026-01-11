/**
 * @file compass.cpp
 * @brief Compass definition and functions - Native ESP-IDF drivers
 * @version 0.2.4
 * @date 2025-12
 */

#include "compass.hpp"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_timer.h"

/**
 * @brief Get system uptime in milliseconds using ESP-IDF timer.
 * @return uint32_t Milliseconds since boot.
 */
static inline uint32_t millis_idf() { return (uint32_t)(esp_timer_get_time() / 1000); }

static const char* TAG = "Compass";

// ============================================================================
// QMC5883L Native Driver Implementation
// ============================================================================

/**
 * @brief Constructs QMC5883L driver with default configuration.
 *
 * @details Initializes with default I2C address and control register value.
 */
QMC5883L_Driver::QMC5883L_Driver() : i2cAddr(QMC5883L_ADDRESS), ctrl1Value(0x01) {}

/**
 * @brief Reads a single byte from a register.
 * @param reg Register address.
 * @return Register value.
 */
uint8_t QMC5883L_Driver::read8(uint8_t reg)
{
    return i2c.read8(i2cAddr, reg);
}

/**
 * @brief Writes a single byte to a register.
 * @param reg Register address.
 * @param value Value to write.
 */
void QMC5883L_Driver::write8(uint8_t reg, uint8_t value)
{
    i2c.write8(i2cAddr, reg, value);
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (LSB first).
 * @param reg Starting register address.
 * @return 16-bit signed value.
 */
int16_t QMC5883L_Driver::read16(uint8_t reg)
{
    uint8_t buffer[2];
    i2c.readBytes(i2cAddr, reg, buffer, 2);
    return (int16_t)(buffer[0] | (buffer[1] << 8));
}

/**
 * @brief Initializes the QMC5883L magnetometer.
 *
 * @details Performs soft reset, configures SET/RESET period, and sets
 *          continuous mode with 10Hz ODR, 2G range, 512 oversampling.
 *
 * @param addr I2C address (default 0x0D).
 * @return true if initialization successful, false otherwise.
 */
bool QMC5883L_Driver::begin(uint8_t addr)
{
    i2cAddr = addr;

    // Soft reset
    write8(QMC5883L_REG_CTRL2, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set/Reset period
    write8(QMC5883L_REG_SET_RST, 0x01);

    // Control register 1: Continuous mode, 10Hz ODR, 2G range, 512x oversampling
    // Bits: OSR[7:6]=00(512), RNG[5:4]=00(2G), ODR[3:2]=00(10Hz), MODE[1:0]=01(Continuous)
    ctrl1Value = 0x01;  // Continuous mode, 10Hz, 2G, 512 OSR
    write8(QMC5883L_REG_CTRL1, ctrl1Value);

    vTaskDelay(pdMS_TO_TICKS(10));

    return true;
}

/**
 * @brief Sets the output data rate.
 *
 * @param rate 0=10Hz, 1=50Hz, 2=100Hz, 3=200Hz.
 */
void QMC5883L_Driver::setDataRate(uint8_t rate)
{
    ctrl1Value = (ctrl1Value & 0xF3) | ((rate & 0x03) << 2);
    write8(QMC5883L_REG_CTRL1, ctrl1Value);
}

/**
 * @brief Sets the oversampling rate.
 *
 * @param samples 0=512, 1=256, 2=128, 3=64.
 */
void QMC5883L_Driver::setSamples(uint8_t samples)
{
    ctrl1Value = (ctrl1Value & 0x3F) | ((samples & 0x03) << 6);
    write8(QMC5883L_REG_CTRL1, ctrl1Value);
}

/**
 * @brief Reads raw magnetometer data.
 *
 * @param x Reference for X-axis raw value.
 * @param y Reference for Y-axis raw value.
 * @param z Reference for Z-axis raw value.
 */
void QMC5883L_Driver::readRaw(float &x, float &y, float &z)
{
    uint8_t buffer[6];
    i2c.readBytes(i2cAddr, QMC5883L_REG_DATA, buffer, 6);

    x = (int16_t)(buffer[0] | (buffer[1] << 8));
    y = (int16_t)(buffer[2] | (buffer[3] << 8));
    z = (int16_t)(buffer[4] | (buffer[5] << 8));
}

// ============================================================================
// HMC5883L Native Driver Implementation
// ============================================================================

/**
 * @brief Constructs HMC5883L driver with default configuration.
 *
 * @details Initializes with default I2C address and config register value.
 */
HMC5883L_Driver::HMC5883L_Driver() : i2cAddr(HMC5883L_ADDRESS), configAValue(0x70) {}

/**
 * @brief Reads a single byte from a register.
 * @param reg Register address.
 * @return Register value.
 */
uint8_t HMC5883L_Driver::read8(uint8_t reg)
{
    return i2c.read8(i2cAddr, reg);
}

/**
 * @brief Writes a single byte to a register.
 * @param reg Register address.
 * @param value Value to write.
 */
void HMC5883L_Driver::write8(uint8_t reg, uint8_t value)
{
    i2c.write8(i2cAddr, reg, value);
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (MSB first).
 * @param reg Starting register address.
 * @return 16-bit signed value.
 */
int16_t HMC5883L_Driver::read16(uint8_t reg)
{
    uint8_t buffer[2];
    i2c.readBytes(i2cAddr, reg, buffer, 2);
    return (int16_t)((buffer[0] << 8) | buffer[1]);
}

/**
 * @brief Initializes the HMC5883L magnetometer.
 *
 * @details Verifies device identity, configures 8 samples average at 15Hz,
 *          and sets continuous measurement mode with default gain.
 *
 * @param addr I2C address (default 0x1E).
 * @return true if initialization successful, false otherwise.
 */
bool HMC5883L_Driver::begin(uint8_t addr)
{
    i2cAddr = addr;

    // Check identification registers (should read 'H', '4', '3')
    uint8_t idA = read8(HMC5883L_REG_ID_A);
    if (idA != 'H')
    {
        ESP_LOGE(TAG, "HMC5883L not found, ID: 0x%02X", idA);
        return false;
    }

    // Config A: 8 samples average, 15Hz, normal measurement
    // Bits: MA[6:5]=11(8 samples), DO[4:2]=100(15Hz), MS[1:0]=00(normal)
    configAValue = 0x70;  // 8 samples, 15Hz
    write8(HMC5883L_REG_CONFIG_A, configAValue);

    // Config B: Gain = 1.3Ga (default)
    write8(HMC5883L_REG_CONFIG_B, 0x20);

    // Mode: Continuous measurement
    write8(HMC5883L_REG_MODE, 0x00);

    vTaskDelay(pdMS_TO_TICKS(10));

    return true;
}

/**
 * @brief Sets the output data rate.
 *
 * @param rate 0=0.75Hz, 1=1.5Hz, 2=3Hz, 3=7.5Hz, 4=15Hz, 5=30Hz, 6=75Hz.
 */
void HMC5883L_Driver::setDataRate(uint8_t rate)
{
    configAValue = (configAValue & 0xE3) | ((rate & 0x07) << 2);
    write8(HMC5883L_REG_CONFIG_A, configAValue);
}

/**
 * @brief Sets the samples average.
 *
 * @param samples 0=1, 1=2, 2=4, 3=8.
 */
void HMC5883L_Driver::setSamples(uint8_t samples)
{
    configAValue = (configAValue & 0x9F) | ((samples & 0x03) << 5);
    write8(HMC5883L_REG_CONFIG_A, configAValue);
}

/**
 * @brief Reads raw magnetometer data.
 *
 * @details Note: HMC5883L data order is X, Z, Y (not X, Y, Z).
 *
 * @param x Reference for X-axis raw value.
 * @param y Reference for Y-axis raw value.
 * @param z Reference for Z-axis raw value.
 */
void HMC5883L_Driver::readRaw(float &x, float &y, float &z)
{
    uint8_t buffer[6];
    i2c.readBytes(i2cAddr, HMC5883L_REG_DATA, buffer, 6);

    // HMC5883L order: X MSB, X LSB, Z MSB, Z LSB, Y MSB, Y LSB
    x = (int16_t)((buffer[0] << 8) | buffer[1]);
    z = (int16_t)((buffer[2] << 8) | buffer[3]);
    y = (int16_t)((buffer[4] << 8) | buffer[5]);
}

// ============================================================================
// MPU9250/AK8963 Native Driver Implementation
// ============================================================================

/**
 * @brief Constructs MPU9250 driver with default configuration.
 *
 * @details Initializes with default I2C addresses for MPU9250 and AK8963.
 */
MPU9250_Driver::MPU9250_Driver()
    : mpuAddr(MPU9250_ADDRESS), akAddr(AK8963_ADDRESS),
      magX(0), magY(0), magZ(0), asaX(1), asaY(1), asaZ(1) {}

/**
 * @brief Reads a single byte from a register.
 * @param addr I2C device address.
 * @param reg Register address.
 * @return Register value.
 */
uint8_t MPU9250_Driver::read8(uint8_t addr, uint8_t reg)
{
    return i2c.read8(addr, reg);
}

/**
 * @brief Writes a single byte to a register.
 * @param addr I2C device address.
 * @param reg Register address.
 * @param value Value to write.
 */
void MPU9250_Driver::write8(uint8_t addr, uint8_t reg, uint8_t value)
{
    i2c.write8(addr, reg, value);
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (LSB first).
 * @param addr I2C device address.
 * @param reg Starting register address.
 * @return 16-bit signed value.
 */
int16_t MPU9250_Driver::read16LE(uint8_t addr, uint8_t reg)
{
    uint8_t buffer[2];
    i2c.readBytes(addr, reg, buffer, 2);
    return (int16_t)(buffer[0] | (buffer[1] << 8));
}

/**
 * @brief Initializes the MPU9250 and AK8963 magnetometer.
 *
 * @details Wakes up MPU9250, enables I2C bypass to access AK8963 directly,
 *          reads sensitivity adjustment values, and configures continuous mode.
 *
 * @param addr MPU9250 I2C address (default 0x68).
 * @return true if initialization successful, false otherwise.
 */
bool MPU9250_Driver::begin(uint8_t addr)
{
    mpuAddr = addr;

    // Check MPU9250 WHO_AM_I
    uint8_t whoAmI = read8(mpuAddr, MPU9250_REG_WHO_AM_I);
    if (whoAmI != 0x71 && whoAmI != 0x73)
    {
        ESP_LOGE(TAG, "MPU9250 not found, WHO_AM_I: 0x%02X", whoAmI);
        return false;
    }

    // Wake up MPU9250
    write8(mpuAddr, MPU9250_REG_PWR_MGMT1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Enable I2C bypass to access AK8963 directly
    write8(mpuAddr, MPU9250_REG_INT_PIN, 0x02);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Check AK8963 WHO_AM_I
    uint8_t akId = read8(akAddr, AK8963_REG_WIA);
    if (akId != 0x48)
    {
        ESP_LOGE(TAG, "AK8963 not found, WIA: 0x%02X", akId);
        return false;
    }

    // Power down AK8963 before changing mode
    write8(akAddr, AK8963_REG_CNTL1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Enter Fuse ROM access mode to read sensitivity adjustment values
    write8(akAddr, AK8963_REG_CNTL1, 0x0F);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read sensitivity adjustment values
    uint8_t rawAsaX = read8(akAddr, AK8963_REG_ASAX);
    uint8_t rawAsaY = read8(akAddr, AK8963_REG_ASAX + 1);
    uint8_t rawAsaZ = read8(akAddr, AK8963_REG_ASAX + 2);

    // Calculate adjustment factors: Hadj = H * ((ASA - 128) * 0.5 / 128 + 1)
    asaX = ((rawAsaX - 128) * 0.5f / 128.0f) + 1.0f;
    asaY = ((rawAsaY - 128) * 0.5f / 128.0f) + 1.0f;
    asaZ = ((rawAsaZ - 128) * 0.5f / 128.0f) + 1.0f;

    // Power down again
    write8(akAddr, AK8963_REG_CNTL1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set continuous measurement mode 2 (100Hz) with 16-bit resolution
    write8(akAddr, AK8963_REG_CNTL1, 0x16);
    vTaskDelay(pdMS_TO_TICKS(10));

    return true;
}

/**
 * @brief Reads magnetometer data from AK8963.
 *
 * @details Checks data ready status and reads 6 bytes of magnetometer data.
 *          Applies sensitivity adjustment to raw values.
 */
void MPU9250_Driver::readSensor()
{
    // Check if data is ready
    uint8_t st1 = read8(akAddr, AK8963_REG_ST1);
    if (!(st1 & 0x01))
        return;

    // Read magnetometer data (6 bytes) + ST2 to complete read cycle
    uint8_t buffer[7];
    i2c.readBytes(akAddr, AK8963_REG_DATA, buffer, 7);

    int16_t rawX = buffer[0] | (buffer[1] << 8);
    int16_t rawY = buffer[2] | (buffer[3] << 8);
    int16_t rawZ = buffer[4] | (buffer[5] << 8);
    // buffer[6] is ST2 register (required to complete read)

    // Apply sensitivity adjustment and convert to microtesla
    // AK8963 scale: 4912 uT for 16-bit mode (32760 counts)
    const float scale = 4912.0f / 32760.0f;
    magX = rawX * asaX * scale;
    magY = rawY * asaY * scale;
    magZ = rawZ * asaZ * scale;
}

/**
 * @brief Gets X-axis magnetic field in microtesla.
 * @return Magnetic field in microtesla (uT).
 */
float MPU9250_Driver::getMagX_uT() { return magX; }

/**
 * @brief Gets Y-axis magnetic field in microtesla.
 * @return Magnetic field in microtesla (uT).
 */
float MPU9250_Driver::getMagY_uT() { return magY; }

/**
 * @brief Gets Z-axis magnetic field in microtesla.
 * @return Magnetic field in microtesla (uT).
 */
float MPU9250_Driver::getMagZ_uT() { return magZ; }

// ============================================================================
// Global Compass Instances
// ============================================================================

#ifdef HMC5883L
    HMC5883L_Driver comp = HMC5883L_Driver();
#endif

#ifdef QMC5883
    QMC5883L_Driver comp = QMC5883L_Driver();
#endif

#ifdef IMU_MPU9250
    MPU9250_Driver IMU = MPU9250_Driver();
#endif

/**
 * @brief Compass class constructor with default filter and calibration values.
 */
Compass::Compass()
        : declinationAngle(0.22f), offX(0.0f), offY(0.0f),
        headingSmooth(0.0f), headingPrevious(0.0f),
        minX(0.0f), maxX(0.0f), minY(0.0f), maxY(0.0f),
        kalmanFilterEnabled(true),
        kalmanFilter(0.01f, 0.1f, 1.0f, 0.0f)
{
}

/**
 * @brief Initializes the compass/magnetometer sensor hardware and configuration.
 */
void Compass::init()
{
#ifdef HMC5883L
    if (!comp.begin())
    {
        ESP_LOGE(TAG, "HMC5883L initialization failed");
        return;
    }
    comp.setDataRate(6);    // 75Hz
    comp.setSamples(0);     // 1 sample
    ESP_LOGI(TAG, "HMC5883L init OK");
#endif

#ifdef QMC5883
    if (!comp.begin())
    {
        ESP_LOGE(TAG, "QMC5883L initialization failed");
        return;
    }
    comp.setDataRate(2);    // 100Hz
    comp.setSamples(2);     // 128 oversampling
    ESP_LOGI(TAG, "QMC5883L init OK");
#endif

#ifdef IMU_MPU9250
    if (!IMU.begin())
    {
        ESP_LOGE(TAG, "MPU9250/AK8963 initialization failed");
        ESP_LOGE(TAG, "Check IMU wiring or try cycling power");
        return;
    }
    ESP_LOGI(TAG, "MPU9250/AK8963 init OK");
#endif
}

/**
 * @brief Reads raw X, Y, Z magnetometer data from the compass sensor.
 * @param x Reference variable for X-axis.
 * @param y Reference variable for Y-axis.
 * @param z Reference variable for Z-axis.
 */
void Compass::read(float &x, float &y, float &z)
{
#ifdef HMC5883L
    comp.readRaw(x, y, z);
#endif

#ifdef QMC5883
    comp.readRaw(x, y, z);
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
 * @brief Calculates the heading (in degrees) from the magnetometer data.
 *
 * @details Applies calibration offsets and applies a Kalman filter if enabled.
 *
 * @return Heading in degrees.
 */
int Compass::getHeading()
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    read(x, y, z);

    float hx = x - offX;
    float hy = y - offY;

    float heading = atan2f(hy, hx);
    heading += declinationAngle;
    heading = wrapToPi(heading);

    if (kalmanFilterEnabled)
    {
        heading = unwrapFromPi(heading, headingPrevious);
        headingSmooth = kalmanFilter.update(heading);
    }
    else
    {
        headingSmooth = heading;
    }

    headingPrevious = heading;

    int headingDeg = static_cast<int>(headingSmooth * (180.0f / M_PI));
    if (headingDeg < 0)
        headingDeg += 360;

    return headingDeg;
}

/**
 * @brief Checks if the compass heading has been updated since the last reading.
 *
 * @details Compares the current heading with the previous value to detect changes.
 *
 * @return true if updated, false otherwise.
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
 * @brief Performs compass calibration routine.
 *
 * @details Guides the user through a calibration process using the screen and touch input.
 * 			Saves the calculated X and Y offsets to persistent configuration.
 */
void Compass::calibrate()
{
    bool cal = 1;
    float y = 0.0f;
    float x = 0.0f;
    float z = 0.0f;
    uint16_t touchX, touchY;

    TFT_eSprite compassCalSprite = TFT_eSprite(&tft);  

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

    compassCalSprite.createSprite(tft.width(), tft.height());
    compassCalSprite.fillScreen(TFT_BLACK);

    compassCalSprite.drawCenterString("ROTATE THE DEVICE", tft.width() >> 1, 10 * scale, fontSmall);
    compassCalSprite.drawPngFile(PSTR("/spiffs/turn.png"), (tft.width() / 2) - 50, 60 * scale);
    compassCalSprite.drawCenterString("TOUCH TO START", tft.width() >> 1, 200 * scale, fontSmall);
    compassCalSprite.drawCenterString("COMPASS CALIBRATION", tft.width() >> 1, 230 * scale, fontSmall);
    compassCalSprite.pushSprite(0,0);

    while (!tft.getTouch(&touchX, &touchY))
    {
    };
    vTaskDelay(pdMS_TO_TICKS(1000));

    unsigned long calTimeWas = millis_idf();

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

        int secmillis = millis_idf() - calTimeWas;
        int secs = (int)((COMPASS_CAL_TIME - secmillis + 1000) / 1000);
        compassCalSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        compassCalSprite.setTextSize(3);
        compassCalSprite.setTextPadding(100);

        char timeString[3] = "";
        memset(&timeString[0], 0, sizeof(timeString));
        sprintf(timeString, "%i", (COMPASS_CAL_TIME - secmillis) / 1000);
        compassCalSprite.drawString(timeString, (tft.width() >> 1), 280 * scale);

        memset(&timeString[0], 0, sizeof(timeString));

        compassCalSprite.pushSprite(0,0);

        if (secs == 0)
        {
            offX = (maxX + minX) / 2;
            offY = (maxY + minY) / 2;
            cal = 0;
        }
    }

    compassCalSprite.setTextSize(1);
    compassCalSprite.drawCenterString("DONE!", tft.width() >> 1, 340 * scale, fontLarge);
    compassCalSprite.drawCenterString("TOUCH TO CONTINUE.", tft.width() >> 1, 380 * scale, fontSmall);

    compassCalSprite.pushSprite(0,0);

    while (!tft.getTouch(&touchX, &touchY))
    {
    };

    compassCalSprite.deleteSprite();

    cfg.saveFloat(PKEYS::KCOMP_OFFSET_X, offX);
    cfg.saveFloat(PKEYS::KCOMP_OFFSET_Y, offY);
}

/**
 * @brief Sets the magnetic declination angle for heading correction.
 * @param angle Declination angle in radians.
 */
void Compass::setDeclinationAngle(float angle)
{
    declinationAngle = angle;
}

/**
 * @brief Sets the calibration offsets for the X and Y axes.
 * @param offsetX X-axis offset.
 * @param offsetY Y-axis offset.
 */
void Compass::setOffsets(float offsetX, float offsetY)
{
    offX = offsetX;
    offY = offsetY;
}

/**
 * @brief Enables or disables the Kalman filter for compass heading smoothing.
 * @param enabled True to enable, false to disable.
 */
void Compass::enableKalmanFilter(bool enabled)
{
    kalmanFilterEnabled = enabled;
}

/**
 * @brief Sets the Kalman filter process and measurement noise constants.
 * @param processNoise Process noise covariance (0-1).
 * @param measureNoise Measurement noise covariance (0-1).
 */
void Compass::setKalmanFilterConst(float processNoise, float measureNoise)
{
    kalmanFilter.setConstants(processNoise, measureNoise);
}

/**
 * @brief Wraps an angle to the range [-pi, pi].
 * @param angle Angle in radians.
 * @return Wrapped angle in radians.
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
 * @brief Unwraps an angle to avoid discontinuity across the [-pi, pi] boundary.
 * @param angle Current angle in radians.
 * @param previousAngle Previous angle in radians.
 * @return Unwrapped angle in radians.
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
