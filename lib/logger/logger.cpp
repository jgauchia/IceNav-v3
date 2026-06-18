/**
 * @file logger.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Data Logger — implementation
 * @version 0.2.9
 * @date 2026-06
 */

#include "logger.hpp"
#include "loggerConfig.hpp"
#include "globalGpxDef.h"
#include "storage.hpp"
#include "esp_timer.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib_noniso.h>
#include "gpsMath.hpp"

GpxLogger gpxLogger;

static constexpr float GAIN_MIN_M = 3.0f;    ///< Min altitude delta counted as gain/loss (m), rejects jitter (Garmin-style threshold)
static constexpr float GAIN_MAX_M = 30.0f;   ///< Max plausible altitude delta between fixes (m), rejects GPS spikes

/**
 * @brief Haversine great-circle distance between two WGS-84 coordinates.
 *
 * @param lat1 Latitude of point 1 (degrees)
 * @param lon1 Longitude of point 1 (degrees)
 * @param lat2 Latitude of point 2 (degrees)
 * @param lon2 Longitude of point 2 (degrees)
 * @return float Distance in metres.
 */
static float loggerDist(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371008.8f;
    const float D = 3.14159265f / 180.0f;
    float dlat = (lat2 - lat1) * D;
    float dlon = (lon2 - lon1) * D;
    float a = sinf(dlat / 2) * sinf(dlat / 2) +
              cosf(lat1 * D) * cosf(lat2 * D) * sinf(dlon / 2) * sinf(dlon / 2);
    return R * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

/**
 * @brief Trim leading spaces left by dtostrf().
 *
 * @param buf Buffer returned by dtostrf().
 * @return const char* Pointer past any leading space characters.
 */
static inline const char* trimDtostrf(const char* buf)
{
    while (*buf == ' ')
        buf++;
    return buf;
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialise the logger: create TRACKS directory and internal mutex.
 */
void GpxLogger::init()
{
    mutex = xSemaphoreCreateMutex();
    if (!storage.exists(trkFolder))
        storage.mkdir(trkFolder);
}

/**
 * @brief Set the active activity profile.
 *
 * @param idx Index into LOGGER_PROFILES (0=walk, 1=bike, 2=car).
 */
void GpxLogger::setProfile(uint8_t idx)
{
    if (idx < LOGGER_PROFILE_COUNT)
        profileIdx = idx;
}

/**
 * @brief Begin a new GPX recording session.
 *
 * @details Called from the UI thread (which holds lvgl_mutex). Opens the GPX
 *          file and writes the header. Does NOT call any LVGL API.
 */
void GpxLogger::start()
{
    if (mutex == nullptr || loggerState != LoggerState::IDLE)
        return;

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE)
        return;

    memset(&trackStats, 0, sizeof(trackStats));
    segOpen       = false;
    hasLast       = false;
    lastGrade     = 0.0f;
    gradeAccDist  = 0.0f;
    gradeAccAlt   = 0.0f;
    flushCnt      = 0;
    pauseStartMs  = 0;
    movingMs      = 0;
    speedAccum    = 0.0f;
    speedSamps    = 0;

    // Filename from system clock (already GPS-synced by gpsTask, local timezone via setenv("TZ"))
    char path[64];
    {
        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);
        snprintf(path, sizeof(path), "%s/%04d%02d%02d_%02d%02d%02d.gpx",
                 trkFolder,
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec);
    }

    strncpy(trackStats.filename, path, sizeof(trackStats.filename) - 1);

    // Extract base name (YYYYMMDD_HHMMSS) — pointer past last '/', strip ".gpx"
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    char trackName[32];
    strncpy(trackName, base, sizeof(trackName) - 1);
    trackName[sizeof(trackName) - 1] = '\0';
    char* dot = strrchr(trackName, '.');
    if (dot)
        *dot = '\0';

    file = storage.open(path, "w");
    if (file == nullptr)
    {
        xSemaphoreGive(mutex);
        return;
    }

    storage.print(file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"IceNav\"\n"
        "  xmlns=\"http://www.topografix.com/GPX/1/1\"\n"
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
        "http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");
    char trkName[48];
    snprintf(trkName, sizeof(trkName), "<trk><name>%s</name>\n", trackName);
    storage.print(file, trkName);

    startMs      = millis_idf();
    lastLogMs    = startMs;
    lastUpdateMs = startMs;
    loggerState        = LoggerState::RECORDING;

    xSemaphoreGive(mutex);
}

/**
 * @brief End the current recording session and close the GPX file.
 *
 * @details Called from the UI thread (which holds lvgl_mutex). Writes the
 *          closing GPX tags and computes final statistics. Does NOT call
 *          any LVGL API (the button callback handles subject updates).
 */
void GpxLogger::stop()
{
    if (mutex == nullptr || loggerState == LoggerState::IDLE)
        return;

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return;

    uint32_t now = millis_idf();

    loggerState = LoggerState::IDLE;

    if (file != nullptr)
    {
        if (segOpen)
            storage.print(file, "</trkseg>\n");
        storage.print(file, "</trk>\n</gpx>\n");
        storage.close(file);
        file = nullptr;
    }

    uint32_t totalMs  = (startMs > 0) ? (now - startMs) : 0;
    trackStats.totalTimeSec  = totalMs / 1000;
    trackStats.movingTimeSec = (movingMs < totalMs) ? movingMs / 1000 : trackStats.totalTimeSec;
    if (speedSamps > 0)
        trackStats.avgSpeedKmh = speedAccum / (float)speedSamps;

    xSemaphoreGive(mutex);
}

/**
 * @brief Hook called from gpsTask on every decoded GPS fix.
 *
 * @details Must be called while gpsMutex is held. Acquires mutex with
 *          timeout 0 to avoid blocking the GPS task. Also tries lvgl_mutex
 *          with timeout 0 when the state changes, to notify the subject.
 *
 * @param gpsFix Snapshot of GPS data filled by gpsTask (avoids gps.hpp dependency).
 */
void GpxLogger::update(const LoggerGpsFix& gpsFix)
{
    if (loggerState == LoggerState::IDLE)
        return;

    if (!gpsFix.valid)
        return;

    if (mutex == nullptr || xSemaphoreTake(mutex, 0) != pdTRUE)
        return;

    uint32_t now      = millis_idf();
    float    lat      = gpsFix.lat;
    float    lon      = gpsFix.lon;
    int16_t  alt      = gpsFix.alt;
    float    speedKmh = gpsFix.speedKmh;

    const LoggerProfile& prof = LOGGER_PROFILES[profileIdx];

    if (loggerState == LoggerState::RECORDING)
    {
        if (speedKmh < prof.pauseSpeedKmh)
        {
            if (pauseStartMs == 0)
                pauseStartMs = now;
            else if ((now - pauseStartMs) >= (prof.pauseDelaySec * 1000U))
            {
                if (segOpen)
                {
                    storage.print(file, "</trkseg>\n");
                    segOpen = false;
                }
                loggerState = LoggerState::AUTO_PAUSE;
                xSemaphoreGive(mutex);
                return;
            }
        }
        else
        {
            pauseStartMs = 0;
            uint32_t delta = now - lastUpdateMs;
            movingMs    += delta;
        }

        uint32_t interval = adaptiveInterval(speedKmh);
        if ((now - lastLogMs) >= interval)
        {
            float dist = hasLast ? loggerDist(lastLat, lastLon, lat, lon) : prof.minDistM + 1.0f;
            if (dist >= prof.minDistM)
            {
                if (!segOpen)
                {
                    storage.print(file, "<trkseg>\n");
                    segOpen = true;
                }
                writeTrkpt(lat, lon, alt, speedKmh, gpsFix);
            }
        }
    }
    else if (loggerState == LoggerState::AUTO_PAUSE)
    {
        if (speedKmh >= prof.pauseSpeedKmh)
        {
            pauseStartMs = 0;
            loggerState        = LoggerState::RECORDING;
        }
    }

    lastUpdateMs = now;
    xSemaphoreGive(mutex);
}

/**
 * @brief Write a single <trkpt> element to the open GPX file.
 *
 * @details Must be called while mutex is held. Updates stats in place.
 *
 * @param lat      Latitude in decimal degrees.
 * @param lon      Longitude in decimal degrees.
 * @param alt      Altitude in metres.
 * @param speedKmh Speed in km/h (converted to m/s in the XML extension).
 * @param gpsFix   GPS snapshot used for the timestamp.
 */
void GpxLogger::writeTrkpt(float lat, float lon, int16_t alt, float speedKmh, const LoggerGpsFix& gpsFix)
{
    char latBuf[14], lonBuf[14], altBuf[12], spdBuf[10];
    dtostrf(lat,             10, 6, latBuf);
    dtostrf(lon,             11, 6, lonBuf);
    dtostrf((float)alt,       6, 1, altBuf);
    dtostrf(speedKmh / 3.6f,  5, 2, spdBuf);

    const char* latS = trimDtostrf(latBuf);
    const char* lonS = trimDtostrf(lonBuf);
    const char* altS = trimDtostrf(altBuf);
    const char* spdS = trimDtostrf(spdBuf);

    char timeBuf[24];
    if (gpsFix.hasTime)
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 (int)gpsFix.year, (int)gpsFix.month, (int)gpsFix.day,
                 (int)gpsFix.hour, (int)gpsFix.minute, (int)gpsFix.second);
    else
    {
        time_t now = time(NULL);
        struct tm t;
        gmtime_r(&now, &t);
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec);
    }

    char line[192];
    snprintf(line, sizeof(line),
             "<trkpt lat=\"%s\" lon=\"%s\"><ele>%s</ele>"
             "<time>%s</time>"
             "<extensions><speed>%s</speed></extensions></trkpt>\n",
             latS, lonS, altS, timeBuf, spdS);

    storage.print(file, line);

    // Update stats
    if (hasLast)
    {
        float dist = loggerDist(lastLat, lastLon, lat, lon);
        trackStats.totalDistM += dist;

        // Elevation gain/loss is band-filtered to reject GPS altitude noise:
        // changes below GAIN_MIN_M are micro-jitter, and changes above GAIN_MAX_M
        // between consecutive fixes are implausible spikes (the AT6558D can jump
        // tens/hundreds of metres on a single fix). Both are ignored, otherwise
        // a short track shows absurd cumulative elevation (e.g. -385 m in 55 m).
        float dAlt    = (float)alt - lastAlt;
        float absAlt  = (dAlt < 0.0f) ? -dAlt : dAlt;
        if (absAlt >= GAIN_MIN_M && absAlt <= GAIN_MAX_M)
        {
            if (dAlt > 0.0f)
                trackStats.gainPos += (int32_t)dAlt;
            else
                trackStats.gainNeg += (int32_t)(-dAlt);
        }

        // Accumulate over a ~30 m window: integer altitude (1 m steps) and short
        // inter-point distances make the instantaneous grade jump wildly, so the
        // grade is only recomputed once enough distance has been covered.
        gradeAccDist += dist;
        gradeAccAlt  += dAlt;
        if (gradeAccDist >= 30.0f)
        {
            lastGrade    = (gradeAccAlt / gradeAccDist) * 100.0f;
            gradeAccDist = 0.0f;
            gradeAccAlt  = 0.0f;
        }
    }

    if (speedKmh > trackStats.maxSpeedKmh)
        trackStats.maxSpeedKmh = speedKmh;

    speedAccum += speedKmh;
    speedSamps++;
    trackStats.numPoints++;

    lastLat = lat;
    lastLon = lon;
    lastAlt = (float)alt;
    hasLast = true;
    lastLogMs = millis_idf();

    flushCnt++;
    if (flushCnt >= 10)
    {
        fflush(file);
        flushCnt = 0;
    }
}

/**
 * @brief Compute the adaptive log interval based on current speed.
 *
 * @details At maxSpeedKmh → minIntervalMs; at rest → maxIntervalMs.
 *
 * @param speedKmh Current speed in km/h.
 * @return uint32_t Interval in milliseconds.
 */
uint32_t GpxLogger::adaptiveInterval(float speedKmh) const
{
    const LoggerProfile& prof = LOGGER_PROFILES[profileIdx];
    float t = speedKmh / prof.maxSpeedKmh;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    return (uint32_t)(prof.maxIntervalMs - t * (float)(prof.maxIntervalMs - prof.minIntervalMs));
}
