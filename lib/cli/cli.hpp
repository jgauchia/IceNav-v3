/**
 * @file cli.hpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version Using https://github.com/hpsaturn/esp32-wifi-cli.git
 * @date 2024-06
 */

#ifndef CLI_HPP
#define CLI_HPP

#ifndef DISABLE_CLI

#include <ESP32WifiCLI.hpp>
#include "utils.h"
#include "gps.hpp"

void initCLI();

#endif

#endif
