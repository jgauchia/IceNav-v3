/**
 * @file Print.h
 * @brief Arduino Print compatibility for ESP-IDF
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#ifdef __cplusplus

// Forward declarations to avoid circular includes
class String;
class __FlashStringHelper;

class Print {
public:
    virtual ~Print() {}

    virtual size_t write(uint8_t) = 0;

    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }

    size_t write(const char *str) {
        if (str == NULL) return 0;
        return write((const uint8_t *)str, strlen(str));
    }

    size_t write(const char *buffer, size_t size) {
        return write((const uint8_t *)buffer, size);
    }

    // Print methods
    size_t print(const char *str) {
        return write(str);
    }

    size_t print(char c) {
        return write((uint8_t)c);
    }

    size_t print(unsigned char b, int base = 10) {
        return print((unsigned long)b, base);
    }

    size_t print(int n, int base = 10) {
        return print((long)n, base);
    }

    size_t print(unsigned int n, int base = 10) {
        return print((unsigned long)n, base);
    }

    size_t print(long n, int base = 10) {
        if (base == 0) {
            return write((uint8_t)n);
        } else if (base == 10) {
            if (n < 0) {
                size_t t = print('-');
                n = -n;
                return printNumber(n, 10) + t;
            }
            return printNumber(n, 10);
        } else {
            return printNumber(n, base);
        }
    }

    size_t print(unsigned long n, int base = 10) {
        if (base == 0) return write((uint8_t)n);
        return printNumber(n, base);
    }

    size_t print(double n, int digits = 2) {
        return printFloat(n, digits);
    }

    size_t print(const String &s);  // Implementation after String is defined

    // FlashStringHelper support (ESP32 doesn't need special handling)
    size_t print(const __FlashStringHelper *str) {
        return print(reinterpret_cast<const char *>(str));
    }

    // Println methods
    size_t println(void) {
        return write("\r\n");
    }

    size_t println(const char *str) {
        size_t n = print(str);
        n += println();
        return n;
    }

    size_t println(char c) {
        size_t n = print(c);
        n += println();
        return n;
    }

    size_t println(unsigned char b, int base = 10) {
        size_t n = print(b, base);
        n += println();
        return n;
    }

    size_t println(int n, int base = 10) {
        size_t n2 = print(n, base);
        n2 += println();
        return n2;
    }

    size_t println(unsigned int n, int base = 10) {
        size_t n2 = print(n, base);
        n2 += println();
        return n2;
    }

    size_t println(long n, int base = 10) {
        size_t n2 = print(n, base);
        n2 += println();
        return n2;
    }

    size_t println(unsigned long n, int base = 10) {
        size_t n2 = print(n, base);
        n2 += println();
        return n2;
    }

    size_t println(double n, int digits = 2) {
        size_t n2 = print(n, digits);
        n2 += println();
        return n2;
    }

    size_t println(const String &s);  // Implementation after String is defined

    size_t println(const __FlashStringHelper *str) {
        size_t n = print(str);
        n += println();
        return n;
    }

    // Printf
    size_t printf(const char *format, ...) __attribute__((format(printf, 2, 3))) {
        char buf[256];
        va_list args;
        va_start(args, format);
        int len = vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        if (len > 0) {
            return write((const uint8_t*)buf, len);
        }
        return 0;
    }

    virtual void flush() {}

protected:
    size_t printNumber(unsigned long n, uint8_t base) {
        char buf[8 * sizeof(long) + 1];
        char *str = &buf[sizeof(buf) - 1];
        *str = '\0';

        if (base < 2) base = 10;

        do {
            unsigned long m = n;
            n /= base;
            char c = m - base * n;
            *--str = c < 10 ? c + '0' : c + 'A' - 10;
        } while (n);

        return write(str);
    }

    size_t printFloat(double number, uint8_t digits) {
        size_t n = 0;

        if (isnan(number)) return print("nan");
        if (isinf(number)) return print("inf");
        if (number > 4294967040.0) return print("ovf");
        if (number < -4294967040.0) return print("ovf");

        if (number < 0.0) {
            n += print('-');
            number = -number;
        }

        double rounding = 0.5;
        for (uint8_t i = 0; i < digits; ++i)
            rounding /= 10.0;
        number += rounding;

        unsigned long int_part = (unsigned long)number;
        double remainder = number - (double)int_part;
        n += print(int_part);

        if (digits > 0) {
            n += print('.');
            while (digits-- > 0) {
                remainder *= 10.0;
                unsigned int toPrint = (unsigned int)remainder;
                n += print(toPrint);
                remainder -= toPrint;
            }
        }

        return n;
    }
};

#endif // __cplusplus
