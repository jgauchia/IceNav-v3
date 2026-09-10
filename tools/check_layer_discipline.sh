#!/usr/bin/env bash
#
# Checks that Layer 2/3 code (lib/gui, lib/router, lib/gpx, lib/utils, lib/maps)
# does not include Layer 0 headers (hal.hpp, tft.hpp/LovyanGFX.hpp, WiFi.h/ESPmDNS.h,
# Arduino.h). See ROADMAP/FASE_01_ARQUITECTURA_MULTIPLATAFORMA.md, "Regla de disciplina".
#
# esp_timer.h and esp_heap_caps.h are accepted exceptions (millis_idf / PSRAM allocators).
# lib/gui/src/globalGuiDef.h is a documented exception for tft.hpp (climbState.sprite /
# splash logo sprite, pending its own stage).

set -euo pipefail

LAYERS="lib/gui lib/router lib/gpx lib/utils lib/maps"
FORBIDDEN='#include[[:space:]]*[<"](hal\.hpp|tft\.hpp|LovyanGFX\.hpp|WiFi\.h|ESPmDNS\.h|Arduino\.h)[>"]'
ALLOWED_FILE="lib/gui/src/globalGuiDef.h"

violations=$(grep -rnE "$FORBIDDEN" $LAYERS --include="*.hpp" --include="*.cpp" --include="*.h" \
    | grep -v "^${ALLOWED_FILE}:" || true)

if [ -n "$violations" ]; then
    echo "Layer discipline violation: Layer 2/3 code includes Layer 0 headers:"
    echo "$violations"
    exit 1
fi

echo "Layer discipline check passed."
