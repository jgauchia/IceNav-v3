/**
 * @file battery.c
 * @brief Battery ADC reading for ESP-IDF
 */

#include "sensors.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "battery";

// ADC configuration
#define BATTERY_ADC_UNIT        ADC_UNIT_2
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_6   // GPIO7 for ADC2_CH6
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_12

// Voltage divider: R1 = 100K, R2 = 100K -> ratio = 2.0
#define VOLTAGE_DIVIDER_RATIO   2.0f
#define ADC_SAMPLES             100

// Reference voltage
#define V_REF                   3.3f

// Battery thresholds
static float battery_max = 4.2f;
static float battery_min = 3.3f;

// ADC handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool is_calibrated = false;

esp_err_t battery_init(void)
{
    ESP_LOGI(TAG, "Initializing battery ADC");

    // ADC unit configuration
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BATTERY_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }

    // Channel configuration
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ret = adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config ADC channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Try to create calibration handle
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = BATTERY_ADC_UNIT,
        .chan = BATTERY_ADC_CHANNEL,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);
    if (ret == ESP_OK) {
        is_calibrated = true;
        ESP_LOGI(TAG, "ADC calibration: curve fitting");
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = BATTERY_ADC_UNIT,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle);
    if (ret == ESP_OK) {
        is_calibrated = true;
        ESP_LOGI(TAG, "ADC calibration: line fitting");
    }
#endif

    if (!is_calibrated) {
        ESP_LOGW(TAG, "ADC calibration not available, using raw values");
    }

    ESP_LOGI(TAG, "Battery ADC initialized");
    return ESP_OK;
}

void battery_set_levels(float max_voltage, float min_voltage)
{
    battery_max = max_voltage;
    battery_min = min_voltage;
    ESP_LOGI(TAG, "Battery levels set: %.2fV - %.2fV", min_voltage, max_voltage);
}

float battery_read_voltage(void)
{
    if (!adc_handle) {
        return -1.0f;
    }

    long sum = 0;
    int valid_samples = 0;

    for (int i = 0; i < ADC_SAMPLES; i++) {
        int raw;
        if (adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw) == ESP_OK) {
            sum += raw;
            valid_samples++;
        }
        vTaskDelay(1);  // Small delay between samples
    }

    if (valid_samples == 0) {
        return -1.0f;
    }

    float voltage;
    int avg_raw = sum / valid_samples;

    if (is_calibrated && cali_handle) {
        int calibrated_mv;
        adc_cali_raw_to_voltage(cali_handle, avg_raw, &calibrated_mv);
        voltage = calibrated_mv / 1000.0f;
    } else {
        // Manual calculation: (raw * Vref) / 4096
        voltage = (avg_raw * V_REF) / 4096.0f;
    }

    // Apply voltage divider compensation
    voltage *= VOLTAGE_DIVIDER_RATIO;

    return voltage;
}

float battery_read_percentage(void)
{
    float voltage = battery_read_voltage();

    if (voltage < 0) {
        return -1.0f;
    }

    float percentage = ((voltage - battery_min) / (battery_max - battery_min)) * 100.0f;

    // Clamp to 0-100
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    return percentage;
}
