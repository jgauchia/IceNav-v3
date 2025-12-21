/**
 * @file HardwareSerial.h
 * @brief Arduino HardwareSerial compatibility for ESP-IDF UART
 */

#pragma once

#include "Stream.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SERIAL_8N1 UART_DATA_8_BITS

class HardwareSerial : public Stream {
public:
    HardwareSerial(uart_port_t uart_num) : _uart_num(uart_num), _rx_buffer_size(1024) {}

    void begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1) {
        _baud = baud;

        uart_config_t uart_config = {
            .baud_rate = (int)baud,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT,
        };

        // Check if UART is already installed
        if (uart_is_driver_installed(_uart_num)) {
            uart_driver_delete(_uart_num);
        }

        ESP_ERROR_CHECK(uart_param_config(_uart_num, &uart_config));

        if (rxPin >= 0 || txPin >= 0) {
            ESP_ERROR_CHECK(uart_set_pin(_uart_num,
                txPin >= 0 ? (gpio_num_t)txPin : UART_PIN_NO_CHANGE,
                rxPin >= 0 ? (gpio_num_t)rxPin : UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE));
        }

        ESP_ERROR_CHECK(uart_driver_install(_uart_num, _rx_buffer_size * 2, 0, 0, NULL, 0));
        _initialized = true;
    }

    void end() {
        if (_initialized) {
            uart_driver_delete(_uart_num);
            _initialized = false;
        }
    }

    void setRxBufferSize(size_t size) {
        _rx_buffer_size = size;
    }

    int available() override {
        if (!_initialized) return 0;
        size_t available_bytes = 0;
        uart_get_buffered_data_len(_uart_num, &available_bytes);
        return (int)available_bytes;
    }

    int read() override {
        if (!_initialized) return -1;
        uint8_t c;
        int len = uart_read_bytes(_uart_num, &c, 1, 0);
        return len > 0 ? c : -1;
    }

    int peek() override {
        // ESP-IDF UART doesn't have native peek, we need to buffer
        if (!_initialized) return -1;
        if (_peek_byte >= 0) return _peek_byte;

        uint8_t c;
        int len = uart_read_bytes(_uart_num, &c, 1, 0);
        if (len > 0) {
            _peek_byte = c;
            return c;
        }
        return -1;
    }

    size_t write(uint8_t c) override {
        if (!_initialized) return 0;
        return uart_write_bytes(_uart_num, (const char*)&c, 1);
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        if (!_initialized) return 0;
        return uart_write_bytes(_uart_num, (const char*)buffer, size);
    }

    void flush() override {
        if (_initialized) {
            uart_wait_tx_done(_uart_num, pdMS_TO_TICKS(100));
        }
    }

    // Override read to handle peek buffer
    size_t readBytes(char *buffer, size_t length) {
        if (!_initialized) return 0;

        size_t count = 0;

        // First, return peeked byte if any
        if (_peek_byte >= 0 && count < length) {
            *buffer++ = (char)_peek_byte;
            _peek_byte = -1;
            count++;
        }

        // Read remaining
        if (count < length) {
            int read_len = uart_read_bytes(_uart_num, (uint8_t*)buffer, length - count, pdMS_TO_TICKS(_timeout));
            if (read_len > 0) count += read_len;
        }

        return count;
    }

    unsigned long getBaudRate() const { return _baud; }

    operator bool() const { return _initialized; }

private:
    uart_port_t _uart_num;
    unsigned long _baud = 0;
    size_t _rx_buffer_size;
    bool _initialized = false;
    int _peek_byte = -1;
};

// Global serial instances (similar to Arduino)
extern HardwareSerial Serial;   // UART0 - USB/Console
extern HardwareSerial Serial1;  // UART1
extern HardwareSerial Serial2;  // UART2
