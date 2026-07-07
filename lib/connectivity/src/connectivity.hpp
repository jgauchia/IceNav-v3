/**
 * @file connectivity.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 connectivity interface
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

/**
 * @class IConnectivity
 * @brief Layer-1 contract for the radio link, agnostic of transport.
 *
 * @details Encapsulates radio bring-up and link state so Layer-2/3 never query
 *          WiFi, Bluetooth nor ESPmDNS directly. The S3 implementation uses
 *          native WiFi; the P4 implementation uses WiFi over an ESP32-C6
 *          co-processor (esp-hosted, SDIO); platforms without radio provide a
 *          no-op stub. The station connection itself is owned by the WiFi
 *          CLI; begin() only brings up link-dependent services (mDNS).
 *          setPins() wires the radio transport (e.g. the C6 SDIO bus) and must
 *          run before the CLI can attempt a connection. Time is synced from
 *          GPS and OTA is performed from SD, so neither belongs here, and
 *          serving files is handled by IFileServer.
 */
class IConnectivity
{
public:
    virtual ~IConnectivity() = default;
    virtual void setPins() {}
    virtual void begin() = 0;
    virtual bool isConnected() = 0;
};

IConnectivity &connectivity();
