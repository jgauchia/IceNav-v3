/**
 * @file power.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 power management interface
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

/**
 * @class IPower
 * @brief Layer-1 contract for device sleep and shutdown.
 *
 * @details Encapsulates the platform sleep/PMIC behaviour so Layer-2/3 never
 *          touch esp_sleep, RTC GPIO holds nor the PMIC directly. The S3
 *          implementation uses light/deep sleep over esp_sleep; a platform with
 *          a discrete PMIC plugs in behind this contract without changing it.
 *          Display brightness lives in IDisplay and battery sampling in
 *          ISensors, so neither belongs here.
 */
class IPower
{
public:
    virtual ~IPower() = default;
    virtual void suspend() = 0;
    virtual void shutdown() = 0;
};

IPower &power();
