/**
 * @file Stream.h
 * @brief Arduino Stream compatibility for ESP-IDF
 */

#pragma once

#include "Print.h"
#include <string.h>

class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;

    // Read methods
    size_t readBytes(char *buffer, size_t length) {
        size_t count = 0;
        while (count < length) {
            int c = read();
            if (c < 0) break;
            *buffer++ = (char)c;
            count++;
        }
        return count;
    }

    size_t readBytes(uint8_t *buffer, size_t length) {
        return readBytes((char *)buffer, length);
    }

    String readString() {
        String ret;
        int c = read();
        while (c >= 0) {
            ret += (char)c;
            c = read();
        }
        return ret;
    }

    String readStringUntil(char terminator) {
        String ret;
        int c = read();
        while (c >= 0 && c != terminator) {
            ret += (char)c;
            c = read();
        }
        return ret;
    }

    // Find methods
    bool find(const char *target) {
        return findUntil(target, strlen(target), NULL, 0);
    }

    bool find(const char *target, size_t length) {
        return findUntil(target, length, NULL, 0);
    }

    bool findUntil(const char *target, const char *terminator) {
        return findUntil(target, strlen(target), terminator, strlen(terminator));
    }

    bool findUntil(const char *target, size_t targetLen, const char *terminator, size_t termLen) {
        if (targetLen == 0) return true;
        size_t index = 0;
        while (true) {
            int c = read();
            if (c < 0) return false;
            if ((char)c == target[index]) {
                if (++index >= targetLen) return true;
            } else {
                index = 0;
            }
            if (termLen > 0 && (char)c == terminator[0]) {
                // Simple terminator check
                return false;
            }
        }
    }

    long parseInt() {
        return parseInt(1); // SKIP_ALL
    }

    long parseInt(int skipChar) {
        bool isNegative = false;
        long value = 0;
        int c;

        // Skip non-numeric
        do {
            c = peek();
            if (c < 0) return 0;
            if (c == '-') break;
            if (c >= '0' && c <= '9') break;
            read();
        } while (true);

        // Check negative
        if (c == '-') {
            isNegative = true;
            read();
            c = peek();
        }

        // Read digits
        while (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
            read();
            c = peek();
        }

        return isNegative ? -value : value;
    }

    float parseFloat() {
        return parseFloat(1);
    }

    float parseFloat(int skipChar) {
        bool isNegative = false;
        float value = 0.0f;
        int c;

        // Skip non-numeric
        do {
            c = peek();
            if (c < 0) return 0;
            if (c == '-' || c == '.' || (c >= '0' && c <= '9')) break;
            read();
        } while (true);

        // Negative
        if (c == '-') {
            isNegative = true;
            read();
            c = peek();
        }

        // Integer part
        while (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
            read();
            c = peek();
        }

        // Decimal part
        if (c == '.') {
            read();
            float fraction = 0.1f;
            c = peek();
            while (c >= '0' && c <= '9') {
                value += (c - '0') * fraction;
                fraction *= 0.1f;
                read();
                c = peek();
            }
        }

        return isNegative ? -value : value;
    }

    void setTimeout(unsigned long timeout) {
        _timeout = timeout;
    }

    unsigned long getTimeout() {
        return _timeout;
    }

protected:
    unsigned long _timeout = 1000;
};
