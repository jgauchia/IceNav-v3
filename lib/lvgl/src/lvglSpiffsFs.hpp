/**
 * @file lvglSpiffsFs.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  SPIFFS file functions for LVGL
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef LVGLSPIFFSFS_HPP
#define LVGLSPIFFSFS_HPP

#include "lvgl.h"
#include <FS.h>
#include <SPIFFS.h>

static void *spiffsFsOpen(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t spiffsFsClose(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t spiffsFsRead(lv_fs_drv_t *drv, void *file_p, void *fileBuf, uint32_t btr, uint32_t *br);
static lv_fs_res_t spiffsFsWrite(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t spiffsFsSeek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t spiffsFsTell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static void *spiffsDirOpen(lv_fs_drv_t *drv, const char *dirPath);
static lv_fs_res_t spiffsDirRead(lv_fs_drv_t *drv, void *dir_p, char *fn, uint32_t fn_len);
static lv_fs_res_t spiffsDirClose(lv_fs_drv_t *drv, void *dir_p);
void lv_port_spiffsFsInit();

#endif