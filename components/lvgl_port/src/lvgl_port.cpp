/**
 * @file lvgl_port.cpp
 * @brief LVGL 9.x port for ESP-IDF with LovyanGFX
 */

#include "lvgl_port.h"
#include "display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "lvgl_port";

// LVGL display and input device
static lv_display_t *lvgl_display = NULL;
static lv_indev_t *lvgl_touch = NULL;

// Draw buffers (in PSRAM for larger size)
static uint8_t *draw_buf1 = NULL;
static uint8_t *draw_buf2 = NULL;

// Mutex for thread safety
static SemaphoreHandle_t lvgl_mutex = NULL;

// LVGL tick timer
static esp_timer_handle_t lvgl_tick_timer = NULL;

// Buffer size (40 lines)
#define BUF_LINES 40

/**
 * @brief Display flush callback for LVGL 9.x
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int x1 = area->x1;
    int y1 = area->y1;
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;

    display_push_colors(x1, y1, w, h, (const uint16_t *)px_map);

    lv_display_flush_ready(disp);
}

/**
 * @brief Touch read callback for LVGL 9.x
 */
static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    int x, y;
    if (display_get_touch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * @brief LVGL tick callback (1ms)
 */
static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

extern "C" {

esp_err_t lvgl_port_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL 9.x port");

    // Create mutex
    lvgl_mutex = xSemaphoreCreateMutex();
    if (!lvgl_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize LVGL
    lv_init();

    // Get display dimensions
    int disp_w = display_width();
    int disp_h = display_height();

    // Allocate draw buffers - try PSRAM first, fallback to internal RAM
    size_t buf_size = disp_w * BUF_LINES * sizeof(lv_color_t);

    // Try PSRAM first
    draw_buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    draw_buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    if (draw_buf1 && draw_buf2) {
        ESP_LOGI(TAG, "Draw buffers: %u bytes each in PSRAM", (unsigned)buf_size);
    } else {
        // Fallback to internal RAM
        if (draw_buf1) heap_caps_free(draw_buf1);
        if (draw_buf2) heap_caps_free(draw_buf2);

        draw_buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
        draw_buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);

        if (!draw_buf1 || !draw_buf2) {
            // Single buffer mode as last resort
            if (draw_buf1) heap_caps_free(draw_buf1);
            if (draw_buf2) heap_caps_free(draw_buf2);
            draw_buf2 = NULL;
            draw_buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
        }

        if (!draw_buf1) {
            ESP_LOGE(TAG, "Failed to allocate draw buffers");
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGW(TAG, "Draw buffers: %u bytes in internal RAM (no PSRAM)", (unsigned)buf_size);
    }

    // Create display
    lvgl_display = lv_display_create(disp_w, disp_h);
    if (!lvgl_display) {
        ESP_LOGE(TAG, "Failed to create display");
        return ESP_FAIL;
    }

    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(lvgl_display, draw_buf1, draw_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display created: %dx%d", disp_w, disp_h);

    // Create touch input device
    lvgl_touch = lv_indev_create();
    if (lvgl_touch) {
        lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(lvgl_touch, lvgl_touch_cb);
        ESP_LOGI(TAG, "Touch input created");
    }

    // Create tick timer (1ms)
    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = false
    };
    esp_err_t ret = esp_timer_create(&tick_timer_args, &lvgl_tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tick timer");
        return ret;
    }
    esp_timer_start_periodic(lvgl_tick_timer, 1000);  // 1ms

    ESP_LOGI(TAG, "LVGL 9.x port initialized");
    return ESP_OK;
}

uint32_t lvgl_port_task_handler(uint32_t max_ms)
{
    return lv_timer_handler();
}

bool lvgl_port_lock(int timeout_ms)
{
    if (!lvgl_mutex) return false;

    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mutex, ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    if (lvgl_mutex) {
        xSemaphoreGive(lvgl_mutex);
    }
}

lv_display_t *lvgl_port_get_display(void)
{
    return lvgl_display;
}

lv_indev_t *lvgl_port_get_touch(void)
{
    return lvgl_touch;
}

} // extern "C"
