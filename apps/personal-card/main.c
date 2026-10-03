#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"

LV_IMAGE_DECLARE(billy_card);

static const char *TAG = "billy_card";
static QueueHandle_t s_keys;
static lv_obj_t *s_battery;

// The shared button timer only enqueues; the main task owns hardware work.
static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    if (event == BSP_BTN_CLICK) (void)xQueueSend(s_keys, &button, 0);
}

static void update_battery(bool available) {
    int soc = available ? bsp_battery_soc() : -1;
    if (!bsp_lvgl_lock(500)) return;
    if (soc < 0) lv_label_set_text(s_battery, "--%");
    else lv_label_set_text_fmt(s_battery, "%d%%", soc);
    bsp_lvgl_unlock();
}

void app_main(void) {
    ESP_LOGI(TAG, "Billy business card starting");
    ESP_ERROR_CHECK(bsp_display_init());
    if (!bsp_lvgl_init()) {
        ESP_LOGE(TAG, "LVGL initialization failed");
        return;
    }
    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Display lock timed out");
        return;
    }
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_t *portrait = lv_image_create(screen);
    lv_image_set_src(portrait, &billy_card);
    lv_obj_center(portrait);
    s_battery = lv_label_create(screen);
    lv_obj_set_style_text_font(s_battery, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_battery, lv_color_hex(0x292929), 0);
    lv_label_set_text(s_battery, "--%");
    lv_obj_align(s_battery, LV_ALIGN_TOP_RIGHT, -25, 9);
    lv_screen_load(screen);
    bsp_lvgl_unlock();

    bool battery_ok = bsp_battery_init() == ESP_OK;
    update_battery(battery_ok);
    s_keys = xQueueCreate(8, sizeof(bsp_btn_t));
    if (!s_keys) {
        ESP_LOGE(TAG, "Input queue allocation failed");
        return;
    }
    esp_err_t button_result = bsp_button_init(on_key, NULL);
    if (button_result != ESP_OK) {
        ESP_LOGE(TAG, "Buttons unavailable: %s", esp_err_to_name(button_result));
    }
    int brightness = 70;
    bool dimmed = false;
    bool blanked = false;
    bsp_display_backlight(brightness);
    int64_t last_input = esp_timer_get_time();
    int64_t last_battery = last_input;
    ESP_LOGI(TAG, "CARD_READY image=%ux%u heap=%u largest=%u buttons=%s",
             (unsigned)billy_card.header.w, (unsigned)billy_card.header.h,
             (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             esp_err_to_name(button_result));

    for (;;) {
        bsp_btn_t key;
        if (xQueueReceive(s_keys, &key, pdMS_TO_TICKS(1000)) == pdTRUE) {
            last_input = esp_timer_get_time();
            if (dimmed || blanked) {
                dimmed = false;
                blanked = false;
            } else if (key == BSP_BTN_OK) {
                blanked = true;
            } else if (key == BSP_BTN_UP && brightness < 100) {
                brightness += 10;
            } else if (key == BSP_BTN_DOWN && brightness > 20) {
                brightness -= 10;
            }
            bsp_display_backlight(blanked ? 0 : brightness);
            ESP_LOGI(TAG, "Key=%d backlight=%d", key, blanked ? 0 : brightness);
        }
        int64_t now = esp_timer_get_time();
        if (!dimmed && !blanked && now - last_input >= 60000000) {
            dimmed = true;
            bsp_display_backlight(10);
        }
        if (now - last_battery >= 60000000) {
            update_battery(battery_ok);
            last_battery = now;
        }
    }
}
