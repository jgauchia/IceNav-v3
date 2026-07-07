/**
 * @file connectivityP4.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-P4 connectivity implementation (WiFi over ESP32-C6 co-processor
 *        via esp-hosted, SDIO + mDNS)
 * @date 2026-07
 */

#include "connectivity.hpp"

#if !defined(RADIO_NONE) && CONFIG_IDF_TARGET_ESP32P4

#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_log.h>
#include "hal.hpp"

static const char *CONN_TAG = "CONNECTIVITY";
static const char *CONN_HOSTNAME = "icenav";

/**
 * @class ConnectivityP4
 * @brief Layer-0 connectivity implementation for ESP32-P4 boards.
 *
 * @details WiFi runs on the ESP32-C6 co-processor over esp-hosted; the P4
 *          talks to it over SDIO. setPins() wires that bus and must run
 *          before the WiFi CLI can attempt a connection. The station link
 *          itself is established by the WiFi CLI; begin() only starts the
 *          services that need an active connection (mDNS).
 */
class ConnectivityP4 : public IConnectivity
{
public:
    void setPins() override
    {
        WiFi.setPins(C6_SDIO_CLK, C6_SDIO_CMD, C6_SDIO_D0, C6_SDIO_D1, C6_SDIO_D2, C6_SDIO_D3, C6_SDIO_RST);
    }

    void begin() override
    {
        if (WiFi.status() != WL_CONNECTED)
            return;

        if (!MDNS.begin(CONN_HOSTNAME))
            ESP_LOGE(CONN_TAG, "mDNS init error");
        else
            ESP_LOGI(CONN_TAG, "mDNS initialized");
    }

    bool isConnected() override
    {
        return WiFi.status() == WL_CONNECTED;
    }
};

/**
 * @brief Provides the P4 connectivity implementation as the Layer-1 singleton.
 */
IConnectivity &connectivity()
{
    static ConnectivityP4 instance;
    return instance;
}

#endif // !RADIO_NONE && CONFIG_IDF_TARGET_ESP32P4
