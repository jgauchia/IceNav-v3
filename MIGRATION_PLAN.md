# IceNav ESP-IDF Migration Plan

## Overview

Migration from Arduino framework to ESP-IDF (pure) for the IceNav GPS Navigator project.

**Branch:** `migration-espidf` (derived from `devel`)

---

## Supported Boards

| Board ID | Description | Flash | PSRAM | MCU |
|----------|-------------|-------|-------|-----|
| ICENAV_BOARD | IceNav Custom Board | 16MB | 8MB OPI | ESP32-S3 |
| TDECK_ESP32S3 | LilyGo T-Deck | 16MB | 8MB OPI | ESP32-S3 |
| ELECROW_ESP32 | Elecrow Terminal | 16MB | 8MB | ESP32-S3 |
| MAKERF_ESP32S3 | MakerFabs ESP32-S3 | - | PSRAM | ESP32-S3 |
| ESP32_N16R4 | ESP32 Wrover Kit | 16MB | 4MB | ESP32 |
| ESP32S3_N16R8 | ESP32-S3 DevKit | 16MB | 8MB | ESP32-S3 |
| T4_S3 | LilyGo T4-S3 AMOLED | 16MB | 8MB | ESP32-S3 |

---

## Project Components

### Core Components
- [ ] **HAL** - Hardware Abstraction Layer (GPIO, pins)
- [ ] **TFT** - Display driver (LovyanGFX)
- [ ] **LVGL** - UI framework
- [ ] **Storage** - SD Card + SPIFFS/LittleFS
- [ ] **Battery** - ADC battery monitoring
- [ ] **Power** - Power management, deep sleep

### Sensors
- [ ] **GPS** - UART GPS module (NeoGPS replacement)
- [ ] **Compass** - HMC5883L / QMC5883
- [ ] **BME280** - Temperature, humidity, pressure
- [ ] **IMU** - MPU6050 / MPU9250

### Features
- [ ] **Maps** - Map rendering
- [ ] **GPX** - GPX file parser
- [ ] **Navigation** - Route navigation
- [ ] **Settings** - NVS preferences
- [ ] **WiFi/CLI** - WiFi management, CLI
- [ ] **WebServer** - Async web server
- [ ] **OTA** - Firmware upgrade

---

## Migration Phases

### Phase 0: Project Setup
- [x] Create ESP-IDF project structure
- [x] Configure CMakeLists.txt
- [x] Setup sdkconfig defaults (ICENAV_BOARD)
- [x] Configure partition tables (16MB)
- [ ] Validate empty project compiles

### Phase 1: Minimal Boot
- [x] Basic main.c with app_main()
- [x] GPIO initialization (board component)
- [x] I2C bus initialization
- [x] SPI bus initialization
- [x] UART initialization
- [x] **Hardware validated**

### Phase 2: Display
- [ ] Integrate LovyanGFX (ESP-IDF compatible)
- [ ] Configure display panels per board
- [ ] Basic display test (fill screen, text)
- [ ] **Hardware validation required**

### Phase 3: LVGL Integration
- [ ] Add LVGL component
- [ ] Configure lv_conf.h for ESP-IDF
- [ ] Display driver integration
- [ ] Touch driver integration
- [ ] Basic LVGL demo screen
- [ ] **Hardware validation required**

### Phase 4: Storage
- [ ] SD Card driver (SPI mode)
- [ ] SPIFFS/LittleFS initialization
- [ ] File operations test
- [ ] **Hardware validation required**

### Phase 5: GPS
- [ ] UART driver for GPS
- [ ] NMEA parser (custom or minmea)
- [ ] GPS data structures
- [ ] GPS task (FreeRTOS)
- [ ] **Hardware validation required**

### Phase 6: Battery & Power
- [ ] ADC driver for battery
- [ ] Battery percentage calculation
- [ ] Power management
- [ ] Deep sleep implementation
- [ ] **Hardware validation required**

### Phase 7: Sensors
- [ ] I2C sensor drivers
- [ ] Compass driver (QMC5883/HMC5883L)
- [ ] BME280 driver
- [ ] IMU driver (MPU6050)
- [ ] **Hardware validation required**

### Phase 8: Settings
- [ ] NVS initialization
- [ ] Settings load/save
- [ ] Default values
- [ ] **Hardware validation required**

### Phase 9: GUI Screens
- [ ] Splash screen
- [ ] Satellite search screen
- [ ] Main screen
- [ ] Settings screens
- [ ] Navigation screen
- [ ] Map screen
- [ ] GPX screens
- [ ] **Hardware validation required per screen**

### Phase 10: Maps & Navigation
- [ ] Map tile loading
- [ ] Map rendering
- [ ] GPX parser (tinyxml2)
- [ ] Navigation logic
- [ ] **Hardware validation required**

### Phase 11: Network Features
- [ ] WiFi driver
- [ ] mDNS
- [ ] HTTP server
- [ ] WebSocket
- [ ] CLI over telnet
- [ ] **Hardware validation required**

### Phase 12: Final Integration
- [ ] OTA updates
- [ ] Full system test
- [ ] Performance optimization
- [ ] Memory usage optimization
- [ ] **Final hardware validation**

---

## Current Status

| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| Phase 0 | COMPLETED | 2025-12-17 | d4d79a2 - ESP-IDF setup validated |
| Phase 1 | COMPLETED | 2025-12-18 | Board component: I2C, SPI, UART init |

---

## File Structure (ESP-IDF)

```
IceNav-v3/
├── CMakeLists.txt
├── sdkconfig.defaults
├── sdkconfig.defaults.esp32s3
├── sdkconfig.defaults.esp32
├── partitions/
│   └── partitions_16MB.csv
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── components/
│   ├── hal/
│   ├── tft/
│   ├── lvgl_port/
│   ├── storage/
│   ├── battery/
│   ├── power/
│   ├── gps/
│   ├── compass/
│   ├── bme280/
│   ├── imu/
│   ├── settings/
│   ├── maps/
│   ├── gpx/
│   ├── navigation/
│   ├── gui/
│   ├── network/
│   └── upgrade/
└── managed_components/
    ├── lvgl/
    └── lovyangfx/
```

---

## Dependencies (ESP-IDF Components)

| Component | Source | Notes |
|-----------|--------|-------|
| LVGL | espressif/lvgl | v9.x |
| LovyanGFX | lovyan03/LovyanGFX | ESP-IDF compatible |
| tinyxml2 | Manual port | For GPX parsing |

---

## Notes

- Each phase requires hardware validation before proceeding
- Commits will be created after each successful validation
- No Arduino compatibility layer will be used
- All drivers will use ESP-IDF native APIs
