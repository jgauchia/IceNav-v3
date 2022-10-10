/**
 * @file png.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  PNG manipulation functions
 * @version 0.1
 * @date 2022-10-10
 */

void load_file(fs::FS &fs, const char *path);
#include "utils/pngle.h"
#include "support_functions.h"
void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]);