/**
 * @file AltSoftSerial.h
 * @brief Dummy AltSoftSerial for ESP-IDF (prevents NeoGPS from auto-selecting it)
 *
 * On ESP32, we use HardwareSerial instead of software serial.
 * This header prevents NeoGPS's GPSport.h from selecting AltSoftSerial.
 */

#pragma once

// Do NOT define AltSoftSerial_h to prevent NeoGPS from using it
// Just provide an empty header so the #include doesn't fail
// NeoGPS will fall back to using Serial
