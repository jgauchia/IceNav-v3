/**
 * @file cli.hpp
 * @author @Hpsaturn
 * @brief  Network CLI and custom internal commands
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#ifndef DISABLE_CLI

#include <ESP32WifiCLI.hpp>
#include "settings.hpp"
#include "utils.h"
#include "gps.hpp"
#include "power.hpp"
#include <NMEAGPS.h>

void initCLI();

#endif
