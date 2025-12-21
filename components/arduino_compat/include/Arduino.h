/**
 * @file Arduino.h
 * @brief Arduino compatibility layer for ESP-IDF
 * @details Provides Arduino-like functions and macros for ESP-IDF environment
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

// Arduino data types
typedef bool boolean;
typedef uint8_t byte;
typedef uint16_t word;

// Time functions
static inline unsigned long millis(void) {
    return (unsigned long)(esp_timer_get_time() / 1000);
}

static inline unsigned long micros(void) {
    return (unsigned long)esp_timer_get_time();
}

static inline void delay(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline void delayMicroseconds(unsigned int us) {
    esp_rom_delay_us(us);
}

// PROGMEM compatibility (ESP32 doesn't need it, flash is memory-mapped)
#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (s)

#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))
#define pgm_read_ptr(addr) (*(const void **)(addr))

#define strlen_P strlen
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strcmp_P strcmp
#define strncmp_P strncmp
#define memcpy_P memcpy

// Math constants
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef TWO_PI
#define TWO_PI 6.28318530717958647692
#endif
#ifndef HALF_PI
#define HALF_PI 1.57079632679489661923
#endif
#ifndef M_PI
#define M_PI PI
#endif

// Constrain macro (safe, no conflict with STL)
#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

// Abs macro (only for C, C++ uses std::abs)
#ifndef __cplusplus
#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif
#endif

// Math conversions
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif

#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

// Bit manipulation
#define bit(b) (1UL << (b))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

// Digital I/O
#define HIGH 1
#define LOW 0
#define INPUT 0x01
#define OUTPUT 0x02
#define INPUT_PULLUP 0x05

// Serial placeholder
#define SERIAL_8N1 0x800001c

// Interrupt control (ESP-IDF uses critical sections)
#define noInterrupts() portDISABLE_INTERRUPTS()
#define interrupts() portENABLE_INTERRUPTS()

// yield() for cooperative multitasking
static inline void yield(void) {
    taskYIELD();
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// Include C++ headers BEFORE defining min/max macros
#include <algorithm>
#include <string>

// __FlashStringHelper - Arduino uses this for PROGMEM strings
// On ESP32, flash is memory-mapped so it's just a marker type
class __FlashStringHelper;

// F() macro - cast to __FlashStringHelper* for compatibility
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(string_literal))
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))

// Arduino-style min/max as template functions (no macro conflict with STL)
template<typename T>
static inline T _arduino_min(T a, T b) { return (a < b) ? a : b; }

template<typename T>
static inline T _arduino_max(T a, T b) { return (a > b) ? a : b; }

// Only define min/max macros if not already defined and if code needs them
// Using templates avoids STL conflicts
#ifndef min
#define min(a, b) _arduino_min(a, b)
#endif
#ifndef max
#define max(a, b) _arduino_max(a, b)
#endif

// Arduino String class (minimal implementation)
class String {
public:
    String() : _str() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const String& s) : _str(s._str) {}
    String(int value) : _str(std::to_string(value)) {}
    String(unsigned int value) : _str(std::to_string(value)) {}
    String(long value) : _str(std::to_string(value)) {}
    String(unsigned long value) : _str(std::to_string(value)) {}
    String(float value, int decimalPlaces = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, value);
        _str = buf;
    }
    String(double value, int decimalPlaces = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, value);
        _str = buf;
    }

    const char* c_str() const { return _str.c_str(); }
    unsigned int length() const { return _str.length(); }
    bool isEmpty() const { return _str.empty(); }

    String& operator=(const String& rhs) { _str = rhs._str; return *this; }
    String& operator=(const char* cstr) { _str = cstr ? cstr : ""; return *this; }

    String operator+(const String& rhs) const { return String((_str + rhs._str).c_str()); }
    String operator+(const char* cstr) const { return String((_str + (cstr ? cstr : "")).c_str()); }
    String& operator+=(const String& rhs) { _str += rhs._str; return *this; }
    String& operator+=(const char* cstr) { if(cstr) _str += cstr; return *this; }
    String& operator+=(char c) { _str += c; return *this; }

    bool operator==(const String& rhs) const { return _str == rhs._str; }
    bool operator==(const char* cstr) const { return _str == (cstr ? cstr : ""); }
    bool operator!=(const String& rhs) const { return _str != rhs._str; }

    char operator[](unsigned int index) const { return _str[index]; }
    char& operator[](unsigned int index) { return _str[index]; }

    int indexOf(char ch, unsigned int fromIndex = 0) const {
        size_t pos = _str.find(ch, fromIndex);
        return pos == std::string::npos ? -1 : (int)pos;
    }

    String substring(unsigned int beginIndex, unsigned int endIndex = 0xFFFFFFFF) const {
        if (endIndex == 0xFFFFFFFF) endIndex = _str.length();
        return String(_str.substr(beginIndex, endIndex - beginIndex).c_str());
    }

    void trim() {
        size_t start = _str.find_first_not_of(" \t\r\n");
        size_t end = _str.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) _str.clear();
        else _str = _str.substr(start, end - start + 1);
    }

    int toInt() const { return atoi(_str.c_str()); }
    float toFloat() const { return atof(_str.c_str()); }
    double toDouble() const { return atof(_str.c_str()); }

private:
    std::string _str;
};

// Include Print and Stream after basic types are defined
#include "Print.h"
#include "Stream.h"

// Inline implementations that require String to be fully defined
inline size_t Print::print(const String &s) {
    return write(s.c_str());
}

inline size_t Print::println(const String &s) {
    size_t n = print(s);
    n += println();
    return n;
}

#endif // __cplusplus
