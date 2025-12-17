/**
 * @file display.cpp
 * @brief Display driver using LovyanGFX for ICENAV_BOARD
 */

#include "display.h"
#include "esp_log.h"
#include <LovyanGFX.hpp>

static const char *TAG = "display";

// =============================================================================
// ICENAV_BOARD Display Configuration
// =============================================================================
#ifdef ICENAV_BOARD

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488   _panel_instance;
    lgfx::Bus_Parallel16  _bus_instance;
    lgfx::Light_PWM       _light_instance;
    lgfx::Touch_FT5x06    _touch_instance;

public:
    LGFX(void)
    {
        // Bus configuration (Parallel 16-bit)
        {
            auto cfg = _bus_instance.config();

            cfg.port = 0;
            cfg.freq_write = 80000000;
            cfg.pin_wr = GPIO_NUM_18;
            cfg.pin_rd = GPIO_NUM_48;
            cfg.pin_rs = GPIO_NUM_45;

            cfg.pin_d0 = GPIO_NUM_47;
            cfg.pin_d1 = GPIO_NUM_21;
            cfg.pin_d2 = GPIO_NUM_14;
            cfg.pin_d3 = GPIO_NUM_13;
            cfg.pin_d4 = GPIO_NUM_12;
            cfg.pin_d5 = GPIO_NUM_11;
            cfg.pin_d6 = GPIO_NUM_10;
            cfg.pin_d7 = GPIO_NUM_9;
            cfg.pin_d8 = GPIO_NUM_3;
            cfg.pin_d9 = GPIO_NUM_8;
            cfg.pin_d10 = GPIO_NUM_16;
            cfg.pin_d11 = GPIO_NUM_15;
            cfg.pin_d12 = GPIO_NUM_7;
            cfg.pin_d13 = GPIO_NUM_6;
            cfg.pin_d14 = GPIO_NUM_5;
            cfg.pin_d15 = GPIO_NUM_4;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs = -1;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;

            cfg.memory_width = 320;
            cfg.memory_height = 480;
            cfg.panel_width = 320;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = true;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }

        // Backlight configuration
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = GPIO_NUM_46;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        // Touch configuration (FT5x06 I2C)
        {
            auto cfg = _touch_instance.config();

            cfg.x_min = 0;
            cfg.x_max = 319;
            cfg.y_min = 0;
            cfg.y_max = 479;
            cfg.pin_int = GPIO_NUM_40;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;

            cfg.i2c_port = 0;
            cfg.i2c_addr = 0x38;
            cfg.pin_sda = GPIO_NUM_38;
            cfg.pin_scl = GPIO_NUM_39;
            cfg.freq = 400000;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

#endif // ICENAV_BOARD

// Global display instance
static LGFX *lcd = nullptr;

extern "C" {

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initializing display");

    lcd = new LGFX();
    if (!lcd) {
        ESP_LOGE(TAG, "Failed to create display instance");
        return ESP_ERR_NO_MEM;
    }

    if (!lcd->init()) {
        ESP_LOGE(TAG, "Display init failed");
        delete lcd;
        lcd = nullptr;
        return ESP_FAIL;
    }

    lcd->setRotation(0);
    lcd->setBrightness(128);
    lcd->fillScreen(TFT_BLACK);

    ESP_LOGI(TAG, "Display OK: %dx%d", (int)lcd->width(), (int)lcd->height());
    return ESP_OK;
}

void display_set_backlight(uint8_t brightness)
{
    if (lcd) {
        lcd->setBrightness(brightness);
    }
}

int display_width(void)
{
    return lcd ? lcd->width() : 0;
}

int display_height(void)
{
    return lcd ? lcd->height() : 0;
}

void display_fill(uint16_t color)
{
    if (lcd) {
        lcd->fillScreen(color);
    }
}

void display_text(int x, int y, const char *text)
{
    if (lcd && text) {
        lcd->setCursor(x, y);
        lcd->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd->setTextSize(2);
        lcd->print(text);
    }
}

} // extern "C"
