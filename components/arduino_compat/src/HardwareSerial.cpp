/**
 * @file HardwareSerial.cpp
 * @brief Arduino HardwareSerial instances for ESP-IDF
 */

#include "Arduino.h"
#include "HardwareSerial.h"

// Global serial instances
HardwareSerial Serial(UART_NUM_0);   // UART0 - USB/Console
HardwareSerial Serial1(UART_NUM_1);  // UART1 - GPS
HardwareSerial Serial2(UART_NUM_2);  // UART2
