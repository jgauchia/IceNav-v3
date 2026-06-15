/**
 * @file mapCanvas.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Layer-1 map drawing surface
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "panelSelect.hpp"

#ifdef T4_S3
#include "LILYGO_T4_S3.hpp"
#endif

#include <LGFX_TFT_eSPI.hpp>

/**
 * Layer-1 map drawing surface.
 *
 * On the ESP32-S3 backend the canvas is a LovyanGFX sprite used as an
 * intermediate framebuffer (draw + blit). A future backend (ESP32-P4) provides
 * a type exposing the same drawing API, mapped onto PPA/2D-DMA. The map
 * renderer (Layer 2) talks to MapCanvas, never to the LovyanGFX type directly.
 */
using MapCanvas = TFT_eSprite;

/**
 * @brief Parent device a MapCanvas is attached to.
 *
 * @details Lets the map renderer create canvases without referencing the
 *          board-specific global display object. Each backend supplies its own.
 */
LovyanGFX *mapCanvasParent();
