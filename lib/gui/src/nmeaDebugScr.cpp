/**
 * @file nmeaDebugScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - NMEA Debug screen (temporary diagnostic tile)
 * @version 0.2.9
 * @date 2026-06
 */

#include "nmeaDebugScr.hpp"
#include "lv_subjects.hpp"
#include "tasks.hpp"
#include "mainScr.hpp"
#include <NMEAGPS.h>
#include "esp_timer.h"
#include <cstring>
#include <cstdio>

extern NMEAGPS GPS;
extern gps_fix  fix;
extern Gps gps;

static lv_obj_t *dbgOkLabel;
static lv_obj_t *dbgErrLabel;
static lv_obj_t *dbgCycleLabel;
static lv_obj_t *dbgLastMsgLabel;
static lv_obj_t *dbgRateLabel;
static lv_obj_t *dbgFixLabel;
static lv_obj_t *dbgSatsLabel;
static lv_obj_t *dbgBufLabel;
static lv_obj_t *dbgHintLabel;
static lv_obj_t *dbgRawLabel;

static uint32_t prevOk    = 0;
static uint32_t prevCycle = 0;
static uint32_t prevMs    = 0;

static const char * msgName(uint8_t id)
{
    switch ((NMEAGPS::nmea_msg_t)id)
    {
        case NMEAGPS::NMEA_UNKNOWN: return "UNKNOWN";
        case NMEAGPS::NMEA_GGA:     return "GGA";
        case NMEAGPS::NMEA_GLL:     return "GLL";
        case NMEAGPS::NMEA_GSA:     return "GSA";
        case NMEAGPS::NMEA_GST:     return "GST";
        case NMEAGPS::NMEA_GSV:     return "GSV";
        case NMEAGPS::NMEA_RMC:     return "RMC";
        case NMEAGPS::NMEA_VTG:     return "VTG";
        case NMEAGPS::NMEA_ZDA:     return "ZDA";
        default:                    return "???";
    }
}

static void nmea_debug_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != DEBUG_NMEA)
        return;

    uint32_t ok, err, cycles;
    uint8_t  lastMsg;

    portENTER_CRITICAL(&nmeaDebugMux);
    ok      = nmeaDebugOk;
    err     = nmeaDebugErr;
    cycles  = nmeaDebugCycles;
    lastMsg = nmeaLastMsg;
    portEXIT_CRITICAL(&nmeaDebugMux);

    lv_label_set_text_fmt(dbgOkLabel,    "Parsed OK : %lu", ok);
    lv_label_set_text_fmt(dbgErrLabel,   "CRC Errors: %lu", err);
    lv_label_set_text_fmt(dbgCycleLabel, "Fix cycles: %lu", cycles);
    lv_label_set_text_fmt(dbgLastMsgLabel,"Last msg  : $%s", msgName(lastMsg));
    lv_label_set_text_fmt(dbgFixLabel,   "Fix status: %d",  (int)fix.status);
    lv_label_set_text_fmt(dbgSatsLabel,  "Sats used : %u",  fix.satellites);

    // Sentences/s and fix-cycles/s measured over the observation window
    uint32_t nowMs = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t dtMs  = nowMs - prevMs;
    if (dtMs >= 2000 && prevMs != 0)
    {
        float sentPerSec  = (ok  - prevOk)    * 1000.0f / (float)dtMs;
        float cyclePerSec = (cycles - prevCycle) * 1000.0f / (float)dtMs;
        lv_label_set_text_fmt(dbgRateLabel,
            "Sent/s: %.1f  Fix/s: %.1f",
            sentPerSec, cyclePerSec);
        prevOk     = ok;
        prevCycle  = cycles;
        prevMs     = nowMs;
    }
    else if (prevMs == 0)
    {
        prevOk     = ok;
        prevCycle  = cycles;
        prevMs     = nowMs;
        lv_label_set_text(dbgRateLabel, "Sent/s: --  Fix/s: --");
    }

    // UART Rx buffer occupancy (Arduino HardwareSerial)
    int avail = gpsPort.available();
    lv_label_set_text_fmt(dbgBufLabel, "UART buf  : %d bytes", avail);

    lv_label_set_text(dbgHintLabel, "Cfg last: $RMC (NMEAGPS_cfg.h)");

    // Raw NMEA: snapshot the ring buffer under the lock, then format outside it
    // to keep the critical section (interrupts disabled) as short as possible.
    static char rawSnap[NMEA_RAW_LINES][NMEA_RAW_LEN];
    uint8_t head;
    portENTER_CRITICAL(&nmeaDebugMux);
    head = nmeaRawHead;
    memcpy(rawSnap, nmeaRawBuf, sizeof(rawSnap));
    portEXIT_CRITICAL(&nmeaDebugMux);

    // static to keep it off the (limited) observer/timer-context stack
    static char rawDump[NMEA_RAW_LINES * NMEA_RAW_LEN];
    size_t pos = 0;
    for (uint8_t i = 0; i < NMEA_RAW_LINES; i++)
    {
        uint8_t idx = (head + i) % NMEA_RAW_LINES;
        if (rawSnap[idx][0] == '\0')
            continue;
        int written = snprintf(rawDump + pos, sizeof(rawDump) - pos, "%s\n", rawSnap[idx]);
        if (written < 0 || (size_t)written >= sizeof(rawDump) - pos)
            break;
        pos += written;
    }
    rawDump[pos] = '\0';
    lv_label_set_text(dbgRawLabel, rawDump);
}

/**
 * @brief NMEA debug screen
 *
 * @details Displays parser statistics: OK/error counts, fix-cycle rate,
 *          last sentence seen, UART buffer occupancy. Helps diagnose
 *          >1Hz GPS failures caused by wrong LAST_SENTENCE_IN_INTERVAL
 *          or buffer overrun.
 *
 * @param screen Pointer to the LVGL tile object.
 */
void nmeaDebugScr(_lv_obj_t *screen)
{
    lv_obj_t *cont = lv_obj_create(screen);
    lv_obj_set_size(cont, TFT_WIDTH, TFT_HEIGHT - 25);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_add_style(cont, &styleTransparent, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    // Stats + raw dump may exceed the tile height; allow vertical scroll.
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);

    auto makeLabel = [&](lv_obj_t *&label, const char *init)
    {
        label = lv_label_create(cont);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_label_set_text(label, init);
    };

    makeLabel(dbgOkLabel,      "Parsed OK : --");
    makeLabel(dbgErrLabel,     "CRC Errors: --");
    makeLabel(dbgCycleLabel,   "Fix cycles: --");
    makeLabel(dbgLastMsgLabel, "Last msg  : --");
    makeLabel(dbgRateLabel,    "Sent/s: --  Fix/s: --");
    makeLabel(dbgFixLabel,     "Fix status: --");
    makeLabel(dbgSatsLabel,    "Sats used : --");
    makeLabel(dbgBufLabel,     "UART buf  : --");
    makeLabel(dbgHintLabel,    "Cfg last: $RMC");

    // Raw NMEA mirror: last sentences fed to the parser, small monospace-ish font
    dbgRawLabel = lv_label_create(cont);
    lv_obj_set_width(dbgRawLabel, TFT_WIDTH - 10);
    lv_obj_set_style_text_font(dbgRawLabel, fontSmall, 0);
    lv_obj_set_style_text_color(dbgRawLabel, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
    lv_label_set_long_mode(dbgRawLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(dbgRawLabel, "");

    lv_subject_add_observer_obj(&subject_nmea_debug_trigger,
                                nmea_debug_observer_cb, cont, NULL);
}
