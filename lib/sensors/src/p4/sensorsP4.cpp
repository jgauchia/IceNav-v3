/**
 * @file sensorsP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 sensors facade implementation
 * @version 0.3.0
 * @date 2026-07
 */

#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4

#include "sensors.hpp"

#ifdef WAVESHARE_P4_35
    // AXP2101 PMIC battery monitor: specific to this board, not a trait of
    // the ESP32-P4 target in general (other P4 boards may not carry it).
    #include "axp2101.hpp"
    extern Axp2101 axp2101;
#endif

#ifdef BME280
    #include "bme.hpp"
    extern BME280_Driver bme;
#endif

/**
 * @class SensorsP4
 * @brief Layer-0 sensors facade for ESP32-P4 boards.
 *
 * @details Battery readings are delegated to the AXP2101 PMIC driver on
 *          boards that carry it (WAVESHARE_P4_35). Ambient readings are
 *          delegated to the BME280 driver, shared with the touch I2C bus.
 *          Compass is not wired up yet on this platform (parked pending a
 *          sensor test PCB), so it reports as absent.
 */
class SensorsP4 : public ISensors
{
public:
    bool hasCompass() const override
    {
        return false;
    }

    bool hasAmbient() const override
    {
        #ifdef BME280
            return true;
        #else
            return false;
        #endif
    }

    bool hasBattery() const override
    {
        #ifdef WAVESHARE_P4_35
            return true;
        #else
            return false;
        #endif
    }

    int heading() override
    {
        return 0;
    }

    bool readAmbient(AmbientData &data) override
    {
        #ifdef BME280
            float t = 0.0f;
            float p = 0.0f;
            float h = 0.0f;
            bme.readAll(t, p, h);
            data.temperature = t;
            data.pressure    = p;
            data.humidity    = h;
            data.altitude    = bme.readAltitude(p);
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
        #ifdef WAVESHARE_P4_35
            return axp2101.readBattery();
        #else
            return 0.0f;
        #endif
    }

    float batteryVoltage() override
    {
        #ifdef WAVESHARE_P4_35
            return axp2101.lastVoltage();
        #else
            return 0.0f;
        #endif
    }

    bool isCharging() override
    {
        #ifdef WAVESHARE_P4_35
            return axp2101.isCharging();
        #else
            return false;
        #endif
    }
};

/**
 * @brief Provides the P4 sensors facade as the Layer-1 singleton.
 */
ISensors &sensors()
{
    static SensorsP4 instance;
    return instance;
}

#endif // CONFIG_IDF_TARGET_ESP32P4
