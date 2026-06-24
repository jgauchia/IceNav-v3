/**
 * @file sensors.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 sensors facade interface
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include <stdint.h>

/**
 * @struct AmbientData
 * @brief Ambient readings produced by an environmental sensor.
 */
struct AmbientData
{
    float temperature;
    float pressure;
    float humidity;
    float altitude;
};

/**
 * @class ISensors
 * @brief Layer-1 facade over the on-board sensors read each cycle.
 *
 * @details Unifies the hot-path readings the logic consumes (heading, ambient,
 *          battery) behind a single contract, delegating to the existing native
 *          drivers (compass/IMU, BME280, battery ADC). Presence of each sensor is
 *          queried at runtime through the has*() methods, so Layer-2/3 no longer
 *          branch on board macros to know what is available. Sensor bring-up,
 *          configuration and calibration stay with the concrete drivers, as they
 *          are start-up concerns, not per-cycle logic.
 */
class ISensors
{
public:
    virtual ~ISensors() = default;

    virtual bool hasCompass() const = 0;
    virtual bool hasAmbient() const = 0;
    virtual bool hasBattery() const = 0;

    /**
     * @brief Current compass heading in degrees, or 0 if no compass.
     */
    virtual int heading() = 0;

    /**
     * @brief Read ambient temperature, pressure, humidity and altitude.
     *
     * @param data Destination for the readings.
     * @return true if an ambient sensor is present and was read.
     */
    virtual bool readAmbient(AmbientData &data) = 0;

    /**
     * @brief Battery charge level (0..100), or 0 if no battery monitor.
     */
    virtual float batteryLevel() = 0;

    /**
     * @brief Last measured battery voltage (V), or 0 if no battery monitor.
     */
    virtual float batteryVoltage() = 0;
};

ISensors &sensors();
