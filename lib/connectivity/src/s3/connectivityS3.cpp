/**
 * @file connectivityS3.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief ESP32-S3 connectivity implementation (native WiFi + mDNS)
 * @date 2026-06
 */

#include "connectivity.hpp"

#if !defined(RADIO_NONE) && !CONFIG_IDF_TARGET_ESP32P4

#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_log.h>

static const char *CONN_TAG = "CONNECTIVITY";
static const char *CONN_HOSTNAME = "icenav";

/**
 * @class ConnectivityS3
 * @brief Layer-0 connectivity implementation for ESP32-S3 boards.
 *
 * @details The station link is established by the WiFi CLI; begin() only starts
 *          the services that need an active connection (mDNS).
 */
class ConnectivityS3 : public IConnectivity
{
public:
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
 * @brief Provides the S3 connectivity implementation as the Layer-1 singleton.
 */
IConnectivity &connectivity()
{
    static ConnectivityS3 instance;
    return instance;
}

#endif // !RADIO_NONE && !CONFIG_IDF_TARGET_ESP32P4
