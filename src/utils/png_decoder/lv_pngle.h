/** \file lv_pngle.h
 *  \brief Header file for LVGL decoder for PNG images using Pngle.
 *
 *  Author: Vincent Paeder
 *  License: MIT
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lv_conf_internal.h"

/** \fn void lv_pngle_init(void)
 *  \brief Initializes the decoder for PNG images using Pngle.
 */
void lv_pngle_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
