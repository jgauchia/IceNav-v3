# How to Add a New Hardware Environment

This document explains how to add support for a new hardware board in IceNav-v3 using the modular `[feat_xxx]` features system in `platformio.ini`.

---

## 1. Identify Your Hardware Features

List which features your board has (battery monitoring, GPS, IMU, compass, BME280, display, etc).

---

## 2. Check for Existing Feature Blocks

Look for existing `[feat_xxx]` blocks in `platformio.ini`.  
If your hardware uses a different pinout or sensor, copy/adapt an existing block and adjust its `build_flags` or `lib_deps`.

Example for a custom ADC channel for battery:
```ini
[feat_battmon_adc2]
build_flags =
    -DADC2
    -DBATT_PIN=ADC2_CHANNEL_6
```

---

## 3. Create a New Environment

Add a new `[env:MY_CUSTOM_BOARD]` section.  
Extend it with `common` and all needed `[feat_xxx]` blocks.  
Set the correct `board` name.

Example (board with battery on ADC2 channel 6, GPS, power save, BME280, MPU6050):

```ini
[env:MY_CUSTOM_BOARD]
extends = common, feat_battmon_adc2, feat_gps_at6558, feat_power, feat_bme280, feat_imu_mpu6050
board = my-custom-board
build_flags =
    ${common.build_flags}
    ${feat_battmon_adc2.build_flags}
    ${feat_gps_at6558.build_flags}
    ${feat_power.build_flags}
    ${feat_bme280.build_flags}
    ${feat_imu_mpu6050.build_flags}
lib_deps =
    ${common.lib_deps}
    ${feat_bme280.lib_deps}
    ${feat_imu_mpu6050.lib_deps}
```

**IMPORTANT:**  
If any `[feat_xxx]` defines its own `lib_deps` or `build_flags`, **concatenate them** in your `[env:...]` as shown above.  
If your environment only uses `[common]`, you can omit both `build_flags` and `lib_deps`.

---

## 4. Special Requirements

If your hardware has special needs (PSRAM, partitions, CPU speed), follow the existing environment examples in `platformio.ini`.

---

## 5. Minimal Example (No Display)

If your hardware has no display, omit any `[feat_scr_...]` in your `extends`:

```ini
[env:MY_HEADLESS_BOARD]
extends = common, feat_battmon, feat_gps_at6558, feat_power
board = my-headless-board
build_flags =
    ${common.build_flags}
    ${feat_battmon.build_flags}
    ${feat_gps_at6558.build_flags}
    ${feat_power.build_flags}
```

---

## 6. Adding Extra Sensors

Just add the required features and concatenate both `build_flags` and `lib_deps` in your environment:

```ini
build_flags =
    ${common.build_flags}
    ${feat_imu_mpu6050.build_flags}
    ${feat_comp_hmc.build_flags}

lib_deps =
    ${common.lib_deps}
    ${feat_imu_mpu6050.lib_deps}
    ${feat_comp_hmc.lib_deps}
```

---

## 7. Adding a New Display

For a new display driver, create a new feature block, e.g.:

```ini
[feat_scr_myscreen]
build_flags = -DMYSCREEN_DRIVER_FLAG
```

And in your environment:

```ini
build_flags =
    ${common.build_flags}
    ${feat_scr_myscreen.build_flags}
```

---

## Summary

- Only combine the features (`feat_xxx`) your hardware actually has.
- **Always concatenate `lib_deps` and `build_flags`** from `[common]` and all features that define them.
- If unsure about pins or flags, check the reference environments.
- Try to keep the modular style so new features are reusable.
- After editing `platformio.ini`, select your environment in PlatformIO and build/upload as usual.

---

**Questions or want to contribute your hardware config?**  
Open a PR or issue in the repository!