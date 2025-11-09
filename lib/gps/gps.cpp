/**
 * @file gps.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPS definition and functions
 * @version 0.2.3
 * @date 2025-11
 */

#include "gps.hpp"
#include "lvgl.h"
#include "widgets.hpp"

extern lv_obj_t *sunriseLabel; 	   /**< Label object for displaying the sunrise time. */
bool setTime = true;        	   /**< Indicates if the system time should be set from GPS. */
bool isGpsFixed = false;           /**< Indicates whether a valid GPS fix has been acquired. */
bool isTimeFixed = false; 	       /**< Indicates whether the system time has been fixed using GPS. */
long gpsBaudDetected = 0;   	   /**< Detected GPS baud rate. */
bool nmea_output_enable = false;   /**< Enables or disables NMEA output. */
gps_fix fix;             	       /**< Latest parsed GPS fix data. */
NMEAGPS GPS;              	       /**< NMEAGPS parser instance. */

static const char* TAG PROGMEM = "GPS";

Gps::Gps() {}


/**
 * @brief Init GPS and custom NMEA parsing.
 *
 * @details Initializes the GPS port with the appropriate baud rate and buffer size.
 * 			If a specific baud rate is not set (gpsBaud != 4), it uses the predefined baud rate array.
 *			Otherwise, it attempts to auto-detect the baud rate.
 */
void Gps::init()
{
    gpsPort.setRxBufferSize(1024);

    if (gpsBaud != 4)
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
        delay(100);

        // Update Rate
        gpsPort.println(GPS_RATE_PCAS[gpsUpdate]);
        gpsPort.flush();
        delay(100);

        // Set NMEA 4.1
        gpsPort.println("$PCAS05,2*1A\r\n");
        gpsPort.flush();
        delay(100);
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
    else if (cfg.getFloat(PKEYS::KLAT_DFL, 0.0) != 0.0)
        return cfg.getFloat(PKEYS::KLAT_DFL, 0.0);
    else
    {
        #ifdef DEFAULT_LAT
            return DEFAULT_LAT;
        #else
            return 0.0;
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
    else if (cfg.getFloat(PKEYS::KLON_DFL, 0.0) != 0.0)
        return cfg.getFloat(PKEYS::KLON_DFL, 0.0);
    else
    {
        #ifdef DEFAULT_LON
            return DEFAULT_LON;
        #else
            return 0.0;
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
        if (!setTime)
        {
            log_v("Get date, time, Sunrise and Sunset");
            // Set ESP RTC - Local time
            String TZ = cfg.isKey(CONFKEYS::KDEF_TZ) ? cfg.getString(CONFKEYS::KDEF_TZ, TZ) : "UTC";
            setLocalTime(fix.dateTime,getPosixTZ(TZ.c_str()));
            // Calculate Sunrise and Sunset only one time when date & time was valid
            calculateSun();
            setTime = true;
            lv_obj_send_event(sunriseLabel, LV_EVENT_VALUE_CHANGED, NULL);
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
        gpsData.hdop = (float)fix.hdop / 1000;
    if (fix.valid.pdop)
        gpsData.pdop = (float)fix.pdop / 1000;
    if (fix.valid.vdop)
        gpsData.vdop = (float)fix.vdop / 1000;

    // // Satellite info
    gpsData.satInView = (uint8_t)GPS.sat_count;
    for (uint8_t i = 0; i < gpsData.satInView; i++)
    {
        satTracker[i].satNum = (uint8_t)GPS.satellites[i].id;
        satTracker[i].elev = (uint8_t)GPS.satellites[i].elevation;
        satTracker[i].azim = (uint16_t)GPS.satellites[i].azimuth;
        satTracker[i].snr = (uint8_t)GPS.satellites[i].snr;
        satTracker[i].active = GPS.satellites[i].tracked;
        strncpy(satTracker[i].talker_id, GPS.satellites[i].talker_id, 3);

        int H = canvasRadius * (90 - satTracker[i].elev) / 90;

        float azimRad = DEG2RAD((float)satTracker[i].azim);
        float sinAzim = lutInit ? sinLUT(azimRad) : sinf(azimRad);
        float cosAzim = lutInit ? cosLUT(azimRad) : cosf(azimRad);

        satTracker[i].posX = canvasCenter_X + H * sinAzim;
        satTracker[i].posY = canvasCenter_Y - H * cosAzim;
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
    pinMode(rxPin, INPUT);     // make sure Serial in is a input pin
    digitalWrite(rxPin, HIGH); // pull up enabled just for noise protection

    for (int i = 0; i < 5; i++)
    {
        x = pulseIn(rxPin, LOW, 125000); // measure the next zero bit width
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
    rate = rate / 3l;
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
 * @brief Check if the speed has changed.
 *
 * @details Compares the current speed with the previous value and updates the previous value if changed.
 *
 * @return true if speed has changed, false otherwise.
 */
bool Gps::isSpeedChanged()
{
    if (gpsData.speed != previousSpeed)
    {
        previousSpeed = gpsData.speed;
        return true;
    }
    return false;
}

/**
 * @brief Check if the altitude has changed.
 *
 * @details Compares the current altitude with the previous value and updates the previous value if changed.
 *
 * @return true if altitude has changed, false otherwise.
 */
bool Gps::isAltitudeChanged()
{
    if (gpsData.altitude != previousAltitude)
    {
        previousAltitude = gpsData.altitude;
        return true;
    }
    return false;
}


/**
 * @brief Check if the latitude or longitude has changed.
 *
 * @details Compares the current latitude and longitude with the previous values and updates them if changed.
 *
 * @return true if latitude or longitude has changed, false otherwise.
 */
bool Gps::hasLocationChange()
{
    if (gpsData.latitude != previousLatitude || gpsData.longitude != previousLongitude)
    {
        previousLatitude = gpsData.latitude;
        previousLongitude = gpsData.longitude;
        return true;
    }
    return false;
}

/**
 * @brief Check if the PDOP, HDOP, or VDOP has changed.
 *
 * @details Compares the current DOP values with the previous values and updates them if changed.
 *
 * @return true if PDOP, HDOP, or VDOP has changed, false otherwise.
 */
bool Gps::isDOPChanged()
{
    if (gpsData.pdop != previousPdop || gpsData.hdop != previousHdop || gpsData.vdop != previousVdop)
    {
        previousPdop = gpsData.pdop;
        previousHdop = gpsData.hdop;
        previousVdop = gpsData.vdop;
        return true;
    }
    return false;
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
void Gps::simFakeGPS(const std::vector<wayPoint>& trackData, uint16_t speed, uint16_t refresh)
{
    if (millis() - lastSimulationTime > refresh) 
    {  
        lastSimulationTime = millis();

        if (simulationIndex < (int)trackData.size() - 2) 
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
                int pointsAdvanced = 0;
                const float maxSegmentDist = 100.0f; // Filter out unrealistic jumps (>100m)
                
                // Add expected distance to accumulated distance
                accumulatedDist += expectedDist;
                
                // Limit to prevent infinite loops
                while (currentIndex < (int)trackData.size() - 1 && pointsAdvanced < 10) 
                { 
                    int nextIndex = currentIndex + 1;
                    float segmentDist = calcDist(trackData[currentIndex].lat, trackData[currentIndex].lon,
                                                trackData[nextIndex].lat, trackData[nextIndex].lon);
                    
                    // Skip unrealistic jumps or duplicate points
                    if (segmentDist > maxSegmentDist || segmentDist < 0.1f) 
                    {
                        currentIndex = nextIndex;
                        continue;
                    }
                    
                    // Check if we can advance to this point
                    if (segmentDist <= accumulatedDist) 
                    {
                        accumulatedDist -= segmentDist;
                        currentIndex = nextIndex;
                        pointsAdvanced++;
                    } 
                    else
                    {
                        break; // Not enough accumulated distance
                    }
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
                    {
                        // Initialize with target heading
                        filteredHeading = targetHeading;
                    }
                    
                    // Normalize final heading
                    if (filteredHeading < 0.0f) filteredHeading += 360.0f;
                    if (filteredHeading >= 360.0f) filteredHeading -= 360.0f;
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
            ESP_LOGI(TAG,"End of GPS signal simulation");
        }
    }
} 
