/**
 * @file lvglSdFs.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  SD file functions for LVGL
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef LVGLSDFS_HPP
#define LVGLSDFS_HPP

#include "lvgl.h"
#include <FS.h>
#include <SD.h>

static void *sdFsOpen(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t sdFsClose(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t sdFsRead(lv_fs_drv_t *drv, void *file_p, void *fileBuf, uint32_t btr, uint32_t *br);
static lv_fs_res_t sdFsWrite(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t sdFsSeek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t sdFsTell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static void *sdDirOpen(lv_fs_drv_t *drv, const char *dirPath);
static lv_fs_res_t sdDirRead(lv_fs_drv_t *drv, void *dir_p, char *fn, uint32_t fn_len);
static lv_fs_res_t sdDirClose(lv_fs_drv_t *drv, void *dir_p);
void lv_port_sdFsInit();

#endif