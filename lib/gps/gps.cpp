/**
 * @file gps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.2.9
 * @date 2026-06
 */

#include "gps.hpp"
#include "../../include/hal.hpp"
#include "lvgl.h"
#include "../gui/src/lv_subjects.hpp"
#include "widgets.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_timer.h"
#include "driver/gpio.h"
#include <SolarCalculator.h>
#include <time.h>

extern RTC_DATA_ATTR time_t rtcSavedTime;
extern RTC_DATA_ATTR bool   rtcTimeValid;

/**
 * @brief Get system uptime in milliseconds using ESP-IDF timer.
 *
 * @return uint32_t Milliseconds since boot.
 */
static inline uint32_t millis_idf() { return (uint32_t)(esp_timer_get_time() / 1000); }

/**
 * @brief Measure pulse width on a GPIO pin (ESP-IDF native)
 *
 * @param pin GPIO pin number
 * @param state State to measure (0=LOW, 1=HIGH)
 * @param timeout Timeout in microseconds
 * @return Pulse width in microseconds, or 0 if timeout
 */
static unsigned long pulseIn_idf(int pin, int state, unsigned long timeout)
{
    gpio_num_t gpio = (gpio_num_t)pin;
    int64_t start = esp_timer_get_time();
    int64_t timeout_us = timeout;

    // Wait for any previous pulse to end
    while (gpio_get_level(gpio) == state)
    {
        if ((esp_timer_get_time() - start) > timeout_us)
            return 0;
    }

    // Wait for pulse to start
    while (gpio_get_level(gpio) != state)
    {
        if ((esp_timer_get_time() - start) > timeout_us)
            return 0;
    }
    int64_t pulseStart = esp_timer_get_time();

    // Wait for pulse to end
    while (gpio_get_level(gpio) == state)
    {
        if ((esp_timer_get_time() - start) > timeout_us)
            return 0;
    }

    return (unsigned long)(esp_timer_get_time() - pulseStart);
}

uint8_t GPS_TX = GPS_TX_DEFAULT;
uint8_t GPS_RX = GPS_RX_DEFAULT;

bool setTime = true;        	   /**< Indicates if the system time should be set from GPS. */
bool isGpsFixed = false;           /**< Indicates whether a valid GPS fix has been acquired. */
long gpsBaudDetected = 0;   	   /**< Detected GPS baud rate. */
bool nmea_output_enable = false;   /**< Enables or disables NMEA output. */
gps_fix fix;             	       /**< Latest parsed GPS fix data. */
NMEAGPS GPS;              	       /**< NMEAGPS parser instance. */
Gps gps;                           /**< Global GPS instance */

static const char* TAG = "GPS";

/**
 * @brief Default constructor for Gps class.
 */
Gps::Gps()
{
    memset(&gpsData, 0, sizeof(GPSDATA));
    memset(&satTracker, 0, sizeof(satTracker));
}

/**
 * @brief Build a $PCAS03 command string with adaptive sentence rates.
 *
 * @details At 9600 baud the link cannot carry the full sentence set (GGA + GSA*3 +
 * 			GSV*3 + RMC, ~595 bytes with fix) faster than ~1Hz. To keep position
 * 			(GGA/RMC) fluid at the selected rate while still delivering DOP and the
 * 			satellite constellation, GSA and GSV are decimated to roughly 1Hz by
 * 			emitting them once every N fixes, where N equals the rate in Hz. The NMEA
 * 			checksum is computed over the payload and appended.
 *
 * @param out     Output buffer for the full "$PCAS03,...*CC\r\n" string.
 * @param outSize Size of the output buffer.
 * @param rateIdx Update rate index into GPS_RATE_HZ.
 */
void buildPcas03(char *out, size_t outSize, uint8_t rateIdx)
{
    uint8_t n = GPS_RATE_HZ[rateIdx];

    char payload[40];
    snprintf(payload, sizeof(payload), "PCAS03,1,0,%u,%u,1,0,0,0,0,0,0", n, n);

    uint8_t crc = 0;
    for (const char *p = payload; *p; ++p)
        crc ^= (uint8_t)*p;

    snprintf(out, outSize, "$%s*%02X\r\n", payload, crc);
}

/**
 * @brief Init GPS and custom NMEA parsing.
 *
 * @details Initializes the GPS port with the appropriate baud rate and buffer size.
 * 			If a specific baud rate is not set (gpsBaud != 3), it uses the predefined baud rate array.
 *			Otherwise, it attempts to auto-detect the baud rate.
 */
void Gps::init()
{
    if (rtcTimeValid && rtcSavedTime > 0)
    {
        struct timeval tv = { .tv_sec = rtcSavedTime, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        setTime = true;
        rtcTimeValid = false;
        ESP_LOGI("GPS", "Time restored from RTC: %lld", (long long)rtcSavedTime);
    }

    gpsPort.setRxBufferSize(2048);

    if (gpsBaud != 3)
        gpsPort.begin(GPS_BAUD[gpsBaud], SERIAL_8N1, GPS_RX, GPS_TX);
    else
    {
        gpsBaudDetected = autoBaud();

        if (gpsBaudDetected != 0)
            gpsPort.begin(gpsBaudDetected, SERIAL_8N1, GPS_RX, GPS_TX);
    }

    #ifdef AT6558D_GPS
        // FACTORY RESET
        // gpsPort.println("$PCAS10,3*1F\r\n");
        // gpsPort.flush();
        // delay(100);

        // GPS
        // gpsPort.println("$PCAS04,1*18\r\n")

        // GPS+GLONASS
        // gpsPort.println("$PCAS04,5*1C\r\n");

        // GPS+BDS+GLONASS
        gpsPort.println("$PCAS04,7*1E\r\n");
        gpsPort.flush();
        vTaskDelay(pdMS_TO_TICKS(100));

        // Update Rate
        gpsPort.println(GPS_RATE_PCAS[gpsUpdate]);
        gpsPort.flush();
        vTaskDelay(pdMS_TO_TICKS(100));

        // Active sentences: GGA + RMC at full rate, GSA + GSV decimated to ~1Hz
        // (disable GLL, VTG, ZDA, TXT). Keeps the 9600 link within budget at >1Hz.
        char pcas03[40];
        buildPcas03(pcas03, sizeof(pcas03), gpsUpdate);
        gpsPort.println(pcas03);
        gpsPort.flush();
        vTaskDelay(pdMS_TO_TICKS(100));

        // Set NMEA 4.1
        gpsPort.println("$PCAS05,2*1A\r\n");
        gpsPort.flush();
        vTaskDelay(pdMS_TO_TICKS(100));

        // Save config to flash
        gpsPort.println("$PCAS00*01\r\n");
        gpsPort.flush();
        vTaskDelay(pdMS_TO_TICKS(200));
    #endif
}

/**
 * @brief Return latitude from GPS or system environment pre-built variable.
 *
 * @details Returns the current latitude using the GPS fix if available, otherwise uses the system configuration
 * 			or a predefined default value. Returns 0.0 if latitude is not defined.
 *
 * @return Latitude value or 0.0 if not defined.
 */
float Gps::getLat()
{
    if (fix.valid.location)
        return fix.latitude();
    else if (cfg.getFloat(PKEYS::KLAT_DFL, 0.0f) != 0.0f)
        return cfg.getFloat(PKEYS::KLAT_DFL, 0.0f);
    else
    {
        #ifdef DEFAULT_LAT
            return DEFAULT_LAT;
        #else
            return 0.0f;
        #endif
    }
}

/**
 * @brief Return longitude from GPS or system environment pre-built variable.
 *
 * @details Returns the current longitude using the GPS fix if available, otherwise uses the system configuration
 * 			or a predefined default value. Returns 0.0 if longitude is not defined.
 *
 * @return Longitude value or 0.0 if not defined.
 */
float Gps::getLon()
{
    if (fix.valid.location)
        return fix.longitude();
    else if (cfg.getFloat(PKEYS::KLON_DFL, 0.0f) != 0.0f)
        return cfg.getFloat(PKEYS::KLON_DFL, 0.0f);
    else
    {
        #ifdef DEFAULT_LON
            return DEFAULT_LON;
        #else
            return 0.0f;
        #endif
    }
}

/**
 * @brief Get GPS parsed data.
 *
 * @details Updates the GPS data structure with the latest parsed values from the GPS fix.
 * 			Handles fix status, satellite information, time/date updates, position, altitude, speed,
 * 			heading, dilution of precision values, and updates satellite tracker positions and status.
 */
void Gps::getGPSData()
{
    // GPS Fix
    if (fix.status != gps_fix::STATUS_NONE)
        isGpsFixed = true;

    // GPS Not fixed
    if (fix.status == gps_fix::STATUS_NONE)
        isGpsFixed = false;

    // Satellite Count
    gpsData.satellites = fix.satellites;

    // Fix Mode
    gpsData.fixMode = fix.status;

    // Time and Date
    if (fix.valid.time && fix.valid.date)
    {
        static uint8_t lastSunDay = 0xFF;
        if (!setTime)
        {
            log_v("Get date, time, Sunrise and Sunset");
            String TZ = cfg.isKey(CONFKEYS::KDEF_TZ) ? cfg.getString(CONFKEYS::KDEF_TZ, "") : "UTC";
            setLocalTime(fix.dateTime, getPosixTZ(TZ.c_str()));
            calculateSun();
            lastSunDay = fix.dateTime.date;
            setTime = true;
            lv_subject_set_int(&subject_sunrise, lv_subject_get_int(&subject_sunrise) + 1);
        }
        else if (fix.dateTime.date != lastSunDay)
        {
            calculateSun();
            lastSunDay = fix.dateTime.date;
            lv_subject_set_int(&subject_sunrise, lv_subject_get_int(&subject_sunrise) + 1);
        }
    }

    // Altitude
    if (fix.valid.altitude)
        gpsData.altitude = fix.alt.whole;

    // Speed
    if (fix.valid.speed)
        gpsData.speed = (uint16_t)fix.speed_kph();

    // Latitude and Longitude
    if (fix.valid.location)
    {
        gpsData.latitude = getLat();
        gpsData.longitude = getLon();
    }

    // Heading
    if (fix.valid.heading)
        gpsData.heading = (uint16_t)fix.heading();

    // HDOP , PDOP , VDOP
    if (fix.valid.hdop)
        gpsData.hdop = fix.hdop / 1000.0f;
    if (fix.valid.pdop)
        gpsData.pdop = fix.pdop / 1000.0f;
    if (fix.valid.vdop)
        gpsData.vdop = fix.vdop / 1000.0f;

    // Satellite info: GSV may be decimated (emitted at ~1Hz while GGA/RMC run
    // at the full rate), so cycles without GSV report sat_count == 0. Keep the
    // last known constellation in those cycles instead of clearing it, so the
    // SNR bars and sky radar stay populated between GSV updates.
    if (GPS.sat_count > 0)
    {
        gpsData.satInView = (uint8_t)GPS.sat_count;
        for (uint8_t i = 0; i < gpsData.satInView; i++)
        {
            satTracker[i].satNum = (uint8_t)GPS.satellites[i].id;
            satTracker[i].elev = (uint8_t)GPS.satellites[i].elevation;
            satTracker[i].azim = (uint16_t)GPS.satellites[i].azimuth;
            satTracker[i].snr = (uint8_t)GPS.satellites[i].snr;
            satTracker[i].active = GPS.satellites[i].tracked;
            strncpy(satTracker[i].talker_id, GPS.satellites[i].talker_id, 3);

            // Clamp elevation between 0 and 90 degrees
            int8_t clampedElev = std::max((int8_t)0, std::min((int8_t)90, (int8_t)satTracker[i].elev));
            int H = canvasRadius * (90 - clampedElev) / 90;

            float azimRad = DEG2RAD((float)satTracker[i].azim);
            float sinAzim = lutInit ? sinLUT(azimRad) : sinf(azimRad);
            float cosAzim = lutInit ? cosLUT(azimRad) : cosf(azimRad);

            satTracker[i].posX = canvasCenter_X + H * sinAzim;
            satTracker[i].posY = canvasCenter_Y - H * cosAzim;
        }
    }

}

/**
 * @brief Detect the baud rate of the incoming GPS signal on a given RX pin.
 *
 * @details Measures the duration of low pulses on the RX line to estimate the baud rate of the connected GPS device.
 * 			Returns the shortest measured pulse width as the likely bit duration.
 *
 * @param rxPin The GPIO pin number used for receiving GPS data.
 * @return long The estimated baud rate (bit duration in microseconds).
 */
long Gps::detectRate(int rxPin)
{
    long rate = 10000, x = 2000;
    gpio_set_direction((gpio_num_t)rxPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)rxPin, GPIO_PULLUP_ONLY);

    for (int i = 0; i < 5; i++)
    {
        x = pulseIn_idf(rxPin, 0, 125000); // measure the next zero bit width
        if (x < 1)
            continue;
        rate = x < rate ? x : rate;
    }
    return rate;
}

/**
 * @brief Detect GPS Baudrate.
 *
 * @details Measures the pulse width on the GPS RX pin multiple times to estimate the baud rate.
 * 			Maps the measured pulse width to the nearest standard baud rate value.
 *
 * @return long Detected baud rate, or 0 if detection failed.
 */
long Gps::autoBaud()
{
    long rate = detectRate(GPS_RX) + detectRate(GPS_RX) + detectRate(GPS_RX);
    rate = rate / 3;
    long baud = 0;
    /*
        Time	Baud Rate
        3333µs (3.3ms)300
        833µs 	1200
        416µs 	2400
        208µs 	4800
        104µs 	9600
        69µs 	14400
        52µs 	19200
        34µs 	28800
        26µs 	38400
        17.3µs 	57600
        8µs 	115200
        Megas min is about 10uS? so there may be some inaccuracy
    */
    if (rate < 12)
        baud = 115200;
    else if (rate < 20)
        baud = 57600;
    else if (rate < 30)
        baud = 38400;
    else if (rate < 40)
        baud = 28800;
    else if (rate < 60)
        baud = 19200;
    else if (rate < 80)
        baud = 14400;
    else if (rate < 150)
        baud = 9600;
    else if (rate < 300)
        baud = 4800;
    else if (rate < 600)
        baud = 2400;
    else if (rate < 1200)
        baud = 1200;
    else
        baud = 0;

    return baud;
}

/**
 * @brief Set system local time from GPS time and timezone.
 *
 * @details Converts the provided GPS time to a struct tm, sets the system time, applies the timezone,
 * 			and logs both the local and UTC time. Also calculates and stores the UTC offset in gpsData.UTC.
 *
 * @param gpsTime The GPS time (NeoGPS::time_t) to set as system time.
 * @param tz The timezone string (POSIX TZ format).
 */
void Gps::setLocalTime(NeoGPS::time_t gpsTime, const char* tz)
{
    struct tm timeinfo;
    timeinfo.tm_year = (2000 + gpsTime.year) - 1900;
    timeinfo.tm_mon = gpsTime.month - 1;
    timeinfo.tm_mday = gpsTime.date;
    timeinfo.tm_hour = gpsTime.hours;
    timeinfo.tm_min = gpsTime.minutes;
    timeinfo.tm_sec = gpsTime.seconds;
    struct timeval now = { .tv_sec = mktime(&timeinfo) };
    settimeofday(&now, NULL);

    setenv("TZ",tz,1);
    tzset();

    time_t tLocal = time(NULL);
    time_t tUTC = time(NULL);
    struct tm local_tm;
    struct tm UTC_tm;
    struct tm *tmLocal = localtime_r(&tLocal, &local_tm);
    struct tm *tmUTC = gmtime_r(&tUTC, &UTC_tm);

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", tmLocal);
    ESP_LOGI(TAG, "Current local time: %s",buffer);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", tmUTC);
    ESP_LOGI(TAG, "Current UTC time: %s", buffer);

    
    int UTC = tmLocal->tm_hour - tmUTC->tm_hour;
    if (UTC > 12) 
        UTC -= 24;
    else if (UTC < -12)
        UTC += 24;
    
    gpsData.UTC  = UTC;

    ESP_LOGI(TAG, "UTC: %i", UTC);
}

/**
 * @brief Simulates a GPS signal over a preloaded track.
 *
 * @details Advances through the provided track data, simulating GPS coordinates and heading.
 *          Applies random offset noise and smoothing to emulate realistic GPS signal behavior.
 *          Updates the simulated GPS data every second if the step distance is above a threshold.
 *
 * @param trackData Vector of wayPoints representing the preloaded GPX track.
 * @param speed Simulated speed in km/h to assign to the GPS data.
 * @param refresh Simulation update rate refresh in ms
 */
void Gps::simFakeGPS(const TrackVector& trackData, uint16_t speed, uint16_t refresh)
{
    if (millis_idf() - lastSimulationTime > refresh)
    {
        lastSimulationTime = millis_idf();

        if (simulationIndex < (int)trackData.size() - 1)
        {
            if (simulationIndex == 0)
            {
                  // --- First point: initialize simulation state ---
                smoothedLat = trackData[0].lat;
                smoothedLon = trackData[0].lon;
                lastSimLat = smoothedLat;
                lastSimLon = smoothedLon;
                filteredHeading = 0.0f;
                accumulatedDist = 0.0f;  // Reset accumulated distance for new track

                gpsData.latitude = smoothedLat;
                gpsData.longitude = smoothedLon;
                gpsData.heading = filteredHeading;
                gpsData.speed = speed;
            }
            else
            {
                float rawLat = trackData[simulationIndex].lat;
                float rawLon = trackData[simulationIndex].lon;

                // Calculate expected distance based on speed and time
                float expectedDist = (speed * 1000.0f) / 3600.0f;  // Convert km/h to m/s
                
                // Advance through track points until we've covered the expected distance
                int currentIndex = simulationIndex;
                const float maxSegmentDist = 5000.0f; // Allow long segments (e.g. 5km)
                
                // Add expected distance to accumulated distance
                accumulatedDist += expectedDist;
                
                // Limit to prevent infinite loops (end of track only)
                while (currentIndex < (int)trackData.size() - 1) 
                { 
                    int nextIndex = currentIndex + 1;
                    float segmentDist = calcDist(trackData[currentIndex].lat, trackData[currentIndex].lon,
                                                trackData[nextIndex].lat, trackData[nextIndex].lon);
                    
                    // Skip duplicate points
                    if (segmentDist < 0.1f) 
                    {
                        currentIndex = nextIndex;
                        continue;
                    }

                    // Skip unrealistic jumps
                    if (segmentDist > maxSegmentDist)
                    {
                        currentIndex = nextIndex;
                        continue;
                    }
                    
                    // Check if we can advance to this point
                    if (segmentDist <= accumulatedDist) 
                    {
                        accumulatedDist -= segmentDist;
                        currentIndex = nextIndex;
                    } 
                    else
                        break; // Not enough accumulated distance
                }
                
                // Update simulation index to the final point
                simulationIndex = currentIndex;
                
                // Update position to the final point
                rawLat = trackData[simulationIndex].lat;
                rawLon = trackData[simulationIndex].lon;

                // --- Apply smoothing BEFORE adding noise ---
                smoothedLat = posAlpha * rawLat + (1.0f - posAlpha) * smoothedLat;
                smoothedLon = posAlpha * rawLon + (1.0f - posAlpha) * smoothedLon;

                // --- Small noise to simulate GPS jitter ---
                float latOffset = random(-3, 3) / 100000.0f;  // Reduced noise for simulation
                float lonOffset = random(-3, 3) / 100000.0f;

                float noisyLat = smoothedLat + latOffset;
                float noisyLon = smoothedLon + lonOffset;

                // --- Realistic heading based on track direction ---
                // Look ahead based on speed (faster = further lookahead)
                int lookAhead = min(max(3, speed / 20), (int)trackData.size() - simulationIndex - 1);
                int targetIdx = simulationIndex + lookAhead;
                
                if (targetIdx < (int)trackData.size()) 
                {
                    // Calculate heading towards future track point
                    float targetHeading = calcCourse(smoothedLat, smoothedLon,
                                                    trackData[targetIdx].lat,
                                                    trackData[targetIdx].lon);
                    
                    if (simulationIndex > 1) 
                    {
                        // Smooth transition to target heading (faster adaptation for higher speeds)
                        float headingDiff = calcAngleDiff(targetHeading, filteredHeading);
                        float adaptationRate = min(0.3f, 0.1f + (speed / 200.0f)); // 0.1-0.3 based on speed
                        filteredHeading += adaptationRate * headingDiff;
                    } 
                    else 
                        // Initialize with target heading
                        filteredHeading = targetHeading;
                    
                    // Normalize final heading
                    if (filteredHeading < 0.0f) 
                        filteredHeading += 360.0f;
                    if (filteredHeading >= 360.0f) 
                        filteredHeading -= 360.0f;
                }

                // --- Final output ---
                gpsData.latitude = noisyLat;
                gpsData.longitude = noisyLon;
                gpsData.heading = filteredHeading;
                gpsData.speed = speed;

                lastSimLat = rawLat;
                lastSimLon = rawLon;
            }
            simulationIndex++;
        }
        else
        {
            gpsData.latitude  = trackData.back().lat;
            gpsData.longitude = trackData.back().lon;
            gpsData.speed     = 0;
        }
    }
}

/**
 * @brief Calculate Sunrise and Sunset based on current GPS position and date.
 */
void calculateSun()
{
    double transit, sunrise, sunset;
    calcSunriseSunset(2000 + fix.dateTime.year,
                        fix.dateTime.month,
                        fix.dateTime.date,
                        gps.gpsData.latitude,
                        gps.gpsData.longitude,
                        transit,
                        sunrise,
                        sunset);
    int hours = (int)sunrise + gps.gpsData.UTC;
    int minutes = (int)round(((sunrise + gps.gpsData.UTC) - hours) * 60);
    snprintf(gps.gpsData.sunriseHour, 6, "%02d:%02d", hours, minutes);
    hours = (int)sunset + gps.gpsData.UTC;
    minutes = (int)round(((sunset + gps.gpsData.UTC) - hours) * 60);
    snprintf(gps.gpsData.sunsetHour, 6, "%02d:%02d", hours, minutes);
    log_i("Sunrise: %s", gps.gpsData.sunriseHour);
    log_i("Sunset: %s", gps.gpsData.sunsetHour);
}
