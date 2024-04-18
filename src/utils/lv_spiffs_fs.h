/**
 * @file lv_spiffs_fs.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  SPIFFS file functions for LVGL
 * @version 0.1.8
 * @date 2024-04
 */

#include "lvgl.h"

/**
 * @brief SPIFFS Open LVGL CallBack
 *
 * @param drv
 * @param path
 * @param mode
 * @return void*
 */
static void *spiffsFsOpen(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);

    const char *flags = "";

    if (mode == LV_FS_MODE_WR)
        flags = FILE_WRITE;
    else if (mode == LV_FS_MODE_RD)
        flags = FILE_READ;
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
        flags = FILE_WRITE;

    File f = SPIFFS.open(path, flags);
    if (!f)
    {
        log_e("Failed to open file! %s",path);
        return NULL;      
    }

    File *lf = new File{f};
   
    // make sure at the beginning
    // fp->seek(0);

    return (void *)lf;
}

/**
 * @brief SPIFFS Close LVGL CallBack
 *
 * @param drv
 * @param file_p
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsFsClose(lv_fs_drv_t *drv, void *file_p)
{
    LV_UNUSED(drv);

    File *fp = (File *)file_p;

    fp->close();

    delete (fp); // when close
    return LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Read LVGL CallBack
 *
 * @param drv
 * @param file_p
 * @param fileBuf
 * @param btr
 * @param br
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsFsRead(lv_fs_drv_t *drv, void *file_p, void *fileBuf, uint32_t btr, uint32_t *br)
{
    LV_UNUSED(drv);

    File *fp = (File *)file_p;

    *br = fp->read((uint8_t *)fileBuf, btr);

    return (int32_t)(*br) < 0 ? LV_FS_RES_UNKNOWN : LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Write LVGL CallBack
 *
 * @param drv
 * @param file_p
 * @param buf
 * @param btw
 * @param bw
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsFsWrite(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    LV_UNUSED(drv);

    File *fp = (File *)file_p;

    *bw = fp->write((const uint8_t *)buf, btw);

    return (int32_t)(*bw) < 0 ? LV_FS_RES_UNKNOWN : LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Seek LVGL CallBack
 *
 * @param drv
 * @param file_p
 * @param pos
 * @param whence
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsFsSeek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);

    File *fp = (File *)file_p;

    SeekMode mode;
    if (whence == LV_FS_SEEK_SET)
        mode = SeekSet;
    else if (whence == LV_FS_SEEK_CUR)
        mode = SeekCur;
    else if (whence == LV_FS_SEEK_END)
        mode = SeekEnd;

    fp->seek(pos, mode);

    return LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Tell LVGL CallBack
 *
 * @param drv
 * @param file_p
 * @param pos_p
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsFsTell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    LV_UNUSED(drv);

    File *fp = (File *)file_p;

    *pos_p = fp->position();

    return LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Dir Open LVGL CallBack
 *
 * @param drv
 * @param dirPath
 * @return void*
 */
static void *spiffsDirOpen(lv_fs_drv_t *drv, const char *dirPath)
{
    LV_UNUSED(drv);

    File root = SPIFFS.open(dirPath);
    if (!root)
    {
        log_e("Failed to open directory!");
        return NULL;
    }

    if (!root.isDirectory())
    {
        log_e("Not a directory!");
        return NULL;
    }

    File *lroot = new File{root};

    return (void *)lroot;
}

/**
 * @brief SPIFFS Dir Read LVGL CallBack
 *
 * @param drv
 * @param dir_p
 * @param fn
 * @param fn_len
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsDirRead(lv_fs_drv_t *drv, void *dir_p, char *fn, uint32_t fn_len)
{
    LV_UNUSED(drv);

    File *root = (File *)dir_p;
    fn[0] = '\0';

    File file = root->openNextFile();
    while (file)
    {
        if (strcmp(file.name(), ".") == 0 || strcmp(file.name(), "..") == 0)
        {
            continue;
        }
        else
        {
            if (file.isDirectory())
            {
                log_v("  DIR :  %s",file.name());
                fn[0] = '/';
                strcpy(&fn[1], file.name());
            }
            else
            {
                log_v("  FILE: %s",file.name());
                log_v("  SIZE: %d",file.size());
                strcpy(fn, file.name());
            }
            break;
        }
        file = root->openNextFile();
    }

    return LV_FS_RES_OK;
}

/**
 * @brief SPIFFS Dir Close LVGL CallBack
 *
 * @param drv
 * @param dir_p
 * @return lv_fs_res_t
 */
static lv_fs_res_t spiffsDirClose(lv_fs_drv_t *drv, void *dir_p)
{
    LV_UNUSED(drv);

    File *root = (File *)dir_p;

    root->close();

    delete (root); // when close

    return LV_FS_RES_OK;
}

/**
 * @brief Init LVGL SPIFFS Filesystem
 *
 */
static void lv_port_spiffsFsInit(void)
{
    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = 'F';
    fs_drv.cache_size = sizeof(File);

    fs_drv.open_cb = spiffsFsOpen;
    fs_drv.close_cb = spiffsFsClose;
    fs_drv.read_cb = spiffsFsRead;
    fs_drv.write_cb = spiffsFsWrite;
    fs_drv.seek_cb = spiffsFsSeek;
    fs_drv.tell_cb = spiffsFsTell;

    fs_drv.dir_close_cb = spiffsDirClose;
    fs_drv.dir_open_cb = spiffsDirOpen;
    fs_drv.dir_read_cb = spiffsDirRead;

    lv_fs_drv_register(&fs_drv);
}
