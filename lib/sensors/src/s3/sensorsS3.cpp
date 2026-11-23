/**
 * @file sensorsS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 sensors facade implementation (native ESP-IDF drivers)
 * @version 0.3.3
 * @date 2026-10
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32S3

#include "sensors.hpp"
#include "battery.hpp"

// compass.hpp derives ENABLE_COMPASS from the board magnetometer/IMU macros,
// so it must be included before that macro is queried below.
#if defined(COMPASS_AUTO) || defined(IMU_MPU9250)
    #include "compass.hpp"
    extern Compass compass;
#endif

#ifdef BME280
    #include "bme.hpp"
    extern BME280_Driver bme;
#endif

#ifdef DPS310
    #include "dps310.hpp"
    extern DPS310_Driver dps310;
#endif

extern Battery battery;

/**
 * @class SensorsS3
 * @brief Layer-0 sensors facade for ESP32-S3 boards.
 *
 * @details Delegates each reading to the existing native driver present on the
 *          board. Absent sensors are resolved at compile time through the board
 *          capability macros and reported via the has*() queries, returning
 *          neutral values so the logic can stay branch-free at runtime.
 */
class SensorsS3 : public ISensors
{
public:
    bool hasCompass() const override
    {
        #ifdef ENABLE_COMPASS
            return true;
        #else
            return false;
        #endif
    }

    bool hasAmbient() const override
    {
        #if defined(BME280)
            return true;
        #elif defined(DPS310)
            return dps310.isDetected();
        #else
            return false;
        #endif
    }

    bool hasBattery() const override
    {
        #ifdef BATT_ADC_UNIT
            return true;
        #else
            return false;
        #endif
    }

    int heading() override
    {
        #ifdef ENABLE_COMPASS
            return compass.getHeading();
        #else
            return 0;
        #endif
    }

    bool readAmbient(AmbientData &data) override
    {
        #if defined(BME280)
            float t = 0.0f;
            float p = 0.0f;
            float h = 0.0f;
            bme.readAll(t, p, h);
            data.temperature = t;
            data.pressure    = p;
            data.humidity    = h;
            data.altitude    = bme.readAltitude(p);
            return true;
        #elif defined(DPS310)
            float t = 0.0f;
            float p = 0.0f;
            float h = 0.0f;
            if (!dps310.isDetected())
            {
                data.temperature = 0.0f;
                data.pressure    = 0.0f;
                data.humidity    = 0.0f;
                data.altitude    = 0.0f;
                return false;
            }
            dps310.readAll(t, p, h);
            // Sanity check: reject grossly out-of-range readings so a bus
            // glitch can't publish nonsense (plausible range 100..1200 hPa)
            if (p < 10000.0f || p > 120000.0f)
            {
                data.temperature = 0.0f;
                data.pressure    = 0.0f;
                data.humidity    = 0.0f;
                data.altitude    = 0.0f;
                return false;
            }
            data.temperature = t;
            data.pressure    = p;
            data.humidity    = h;
            data.altitude    = dps310.readAltitude(p);
            return true;
        #else
            data.temperature = 0.0f;
            data.pressure    = 0.0f;
            data.humidity    = 0.0f;
            data.altitude    = 0.0f;
            return false;
        #endif
    }

    float batteryLevel() override
    {
        #ifdef BATT_ADC_UNIT
            return battery.readBattery();
        #else
            return 0.0f;
        #endif
    }

    float batteryVoltage() override
    {
        #ifdef BATT_ADC_UNIT
            return battery.lastVoltage();
        #else
            return 0.0f;
        #endif
    }

    bool isCharging() override
    {
        return battery.isChargingInferred();
    }
};

/**
 * @brief Provides the S3 sensors facade as the Layer-1 .
 */
ISensors &sensors()
{
    static SensorsS3 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32S3
