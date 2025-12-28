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

static const char* TAG PROGMEM = "Compass";

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
void QMC5883L_Driver::write8(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

/**
 * @brief Reads a 16-bit value from two consecutive registers (LSB first).
 * @param reg Starting register address.
 * @return 16-bit signed value.
 */
int16_t QMC5883L_Driver::read16(uint8_t reg)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)2);
    int16_t value = Wire.read();        // LSB first
    value |= (Wire.read() << 8);        // MSB
    return value;
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
    Wire.beginTransmission(i2cAddr);
    Wire.write(QMC5883L_REG_DATA);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)6);

    x = (int16_t)(Wire.read() | (Wire.read() << 8));
    y = (int16_t)(Wire.read() | (Wire.read() << 8));
    z = (int16_t)(Wire.read() | (Wire.read() << 8));
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
void HMC5883L_Driver::write8(uint8_t reg, uint8_t value)
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
int16_t HMC5883L_Driver::read16(uint8_t reg)
{
    Wire.beginTransmission(i2cAddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)2);
    int16_t value = Wire.read() << 8;   // MSB first
    value |= Wire.read();               // LSB
    return value;
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
    Wire.beginTransmission(i2cAddr);
    Wire.write(HMC5883L_REG_DATA);
    Wire.endTransmission(false);
    Wire.requestFrom(i2cAddr, (uint8_t)6);

    // HMC5883L order: X MSB, X LSB, Z MSB, Z LSB, Y MSB, Y LSB
    x = (int16_t)((Wire.read() << 8) | Wire.read());
    z = (int16_t)((Wire.read() << 8) | Wire.read());
    y = (int16_t)((Wire.read() << 8) | Wire.read());
}

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
    MPU9250 IMU = MPU9250(Wire, 0x68);
#endif

/**
 * @brief Compass class constructor with default filter and calibration values.
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
    int status = IMU.begin();
    if (status < 0)
    {
        ESP_LOGE(TAG, "IMU initialization unsuccessful");
        ESP_LOGE(TAG, "Check IMU wiring or try cycling power");
        ESP_LOGE(TAG, "Status: %i", status);
    }
    else
    {
        ESP_LOGI(TAG, "MPU9250 init OK");
    }
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
    float y = 0.0;
    float x = 0.0;
    float z = 0.0;
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
