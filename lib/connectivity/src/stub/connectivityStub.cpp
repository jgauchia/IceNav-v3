/**
 * @file connectivityStub.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief No-op connectivity implementation for platforms without radio
 * @date 2026-06
 */

#include "connectivity.hpp"

#if defined(RADIO_NONE)

/**
 * @class ConnectivityStub
 * @brief Layer-0 no-op connectivity for platforms lacking a radio.
 *
 * @details Compiled when RADIO_NONE is defined (e.g. an ESP32-P4 board without a
 *          radio co-processor). The link is always reported as down so every
 *          radio-dependent service degrades gracefully.
 */
class ConnectivityStub : public IConnectivity
{
public:
    void begin() override {}

    bool isConnected() override
    {
        return false;
    }
};

/**
 * @brief Provides the no-op connectivity implementation as the Layer-1 singleton.
 */
IConnectivity &connectivity()
{
    static ConnectivityStub instance;
    return instance;
}

#endif // RADIO_NONE
