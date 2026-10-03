#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "tuner_pitch.h"
#include "metronome.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "tuner";
static QueueHandle_t s_keys;
static lv_obj_t *s_note, *s_frequency, *s_cents, *s_status, *s_reference, *s_needle, *s_battery;
static lv_obj_t *s_string_tiles[6];
static lv_obj_t *s_string_names[6], *s_mode, *s_menu, *s_menu_rows[3];
static lv_obj_t *s_tuner_screen, *s_metro_screen, *s_metro_battery;
static lv_obj_t *s_bpm, *s_volume_label, *s_metro_help, *s_metro_status, *s_pendulum, *s_beats[4];
static lv_point_precise_t s_tuner_needle_points[2] = {{80, 39}, {80, 0}};
static lv_point_precise_t s_pendulum_points[2] = {{80, 88}, {80, 10}};
typedef struct { bsp_btn_t key; bsp_btn_ev_t event; } key_event_t;
typedef struct { const char *name; int count; const char *labels[6]; int notes[6]; } tuning_t;
static const tuning_t s_tunings[] = {
    {"GUITAR 6", 6, {"E", "A", "D", "G", "B", "e"}, {40, 45, 50, 55, 59, 64}},
    {"BASS 4", 4, {"E", "A", "D", "G"}, {28, 33, 38, 43}},
    {"VIOLIN 4", 4, {"G", "D", "A", "E"}, {55, 62, 69, 76}},
};
static int s_tuning;

#define TUNER_NAVY 0x071C35
#define TUNER_CREAM 0xFFF1CC
#define TUNER_MUTED 0xAFC5E1
#define TUNER_MINT 0x75F2BE
#define TUNER_CORAL 0xFF796F
#define TUNER_AMBER 0xFFCB65

static lv_obj_t *shape(lv_obj_t *parent, int x, int y, int w, int h,
                       int radius, uint32_t color) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label(lv_obj_t *screen, const char *text, int y, const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(screen);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y);
    return obj;
}

static void build_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(TUNER_NAVY), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    shape(screen, 42, 18, 34, 1, 0, 0x58799F);
    shape(screen, 164, 18, 34, 1, 0, 0x58799F);
    label(screen, "TUNER", 10, &lv_font_montserrat_14, TUNER_CREAM);
    s_battery = label(screen, "--%", 11, &lv_font_montserrat_14, TUNER_MINT);
    lv_obj_align(s_battery, LV_ALIGN_TOP_RIGHT, -21, 11);

    s_note = label(screen, "--", 30, &lv_font_montserrat_48, TUNER_CREAM);
    s_frequency = label(screen, "-- Hz", 87, &lv_font_montserrat_20, TUNER_MUTED);
    s_mode = label(screen, "GUITAR 6  v", 111, &lv_font_montserrat_14, TUNER_MINT);

    // The segment heights follow a shallow arch; the movable needle and
    // fixed center tick are separate, as in the approved visual reference.
    for (int i = 0; i < 11; ++i) {
        int distance = i - 5;
        int y = 134 + distance * distance * 4 / 5;
        uint32_t color = i < 4 ? TUNER_CORAL : i > 6 ? TUNER_AMBER : TUNER_MINT;
        shape(screen, 20 + i * 18, y, 16, 20, 3, color);
    }
    shape(screen, 118, 130, 4, 31, 2, TUNER_CREAM); // fixed zero tick
    s_needle = lv_line_create(screen);
    lv_obj_set_pos(s_needle, 40, 145);
    lv_obj_set_style_line_color(s_needle, lv_color_hex(TUNER_MINT), 0);
    lv_obj_set_style_line_width(s_needle, 4, 0);
    lv_obj_set_style_line_rounded(s_needle, true, 0);
    lv_line_set_points(s_needle, s_tuner_needle_points, 2);
    shape(screen, 114, 178, 12, 12, 6, TUNER_CREAM); // needle hub
    lv_obj_add_flag(s_needle, LV_OBJ_FLAG_HIDDEN);

    lv_obj_align(label(screen, "-", 189, &lv_font_montserrat_20, TUNER_CORAL),
                 LV_ALIGN_TOP_LEFT, 20, 189);
    lv_obj_align(label(screen, "+", 189, &lv_font_montserrat_20, TUNER_AMBER),
                 LV_ALIGN_TOP_RIGHT, -20, 189);
    s_cents = label(screen, "-- cents", 190, &lv_font_montserrat_20, TUNER_CORAL);
    s_status = label(screen, "PLUCK ONE STRING", 217, &lv_font_montserrat_14, TUNER_CREAM);
    label(screen, "HOLD UP: METRONOME", 237, &lv_font_montserrat_14, TUNER_MUTED);

    for (int i = 0; i < 6; ++i) {
        s_string_tiles[i] = shape(screen, 9 + i * 38, 254, 33, 32, 8, 0x142B48);
        lv_obj_set_style_border_width(s_string_tiles[i], 1, 0);
        lv_obj_set_style_border_color(s_string_tiles[i], lv_color_hex(0x355477), 0);
        lv_obj_t *name = lv_label_create(s_string_tiles[i]);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(TUNER_CREAM), 0);
        lv_label_set_text(name, s_tunings[0].labels[i]);
        lv_obj_center(name);
        s_string_names[i] = name;
    }
    s_reference = label(screen, "A4 440 Hz  |  HOLD OK: MODE", 293, &lv_font_montserrat_14, TUNER_MUTED);
    s_menu = shape(screen, 28, 105, 184, 133, 12, 0x142B48);
    lv_obj_set_style_border_width(s_menu, 2, 0);
    lv_obj_set_style_border_color(s_menu, lv_color_hex(TUNER_MINT), 0);
    for (int i = 0; i < 3; ++i) {
        s_menu_rows[i] = label(s_menu, s_tunings[i].name, 13 + i * 39,
                               &lv_font_montserrat_20, TUNER_CREAM);
    }
    lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    s_tuner_screen = screen;
    lv_screen_load(screen);
}

static void build_metronome_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(TUNER_NAVY), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    shape(screen, 30, 18, 36, 1, 0, 0x58799F);
    shape(screen, 174, 18, 36, 1, 0, 0x58799F);
    label(screen, "METRONOME", 10, &lv_font_montserrat_14, TUNER_CREAM);
    s_metro_battery = label(screen, "--%", 11, &lv_font_montserrat_14, TUNER_MINT);
    lv_obj_align(s_metro_battery, LV_ALIGN_TOP_RIGHT, -21, 11);
    s_bpm = label(screen, "120", 36, &lv_font_montserrat_48, TUNER_MINT);
    label(screen, "BPM", 92, &lv_font_montserrat_20, TUNER_MUTED);
    s_volume_label = label(screen, "VOL 65%", 110, &lv_font_montserrat_14, TUNER_MUTED);
    shape(screen, 118, 128, 4, 75, 2, 0x355477);
    s_pendulum = lv_line_create(screen);
    lv_obj_set_pos(s_pendulum, 40, 120);
    lv_obj_set_style_line_color(s_pendulum, lv_color_hex(TUNER_MINT), 0);
    lv_obj_set_style_line_width(s_pendulum, 6, 0);
    lv_obj_set_style_line_rounded(s_pendulum, true, 0);
    lv_line_set_points(s_pendulum, s_pendulum_points, 2);
    shape(screen, 112, 203, 16, 16, 8, TUNER_CREAM);
    for (int i = 0; i < 4; ++i) s_beats[i] = shape(screen, 46 + 39 * i, 234, 24, 24, 12, 0x355477);
    s_metro_status = label(screen, "OK TO START", 266, &lv_font_montserrat_14, TUNER_CREAM);
    s_metro_help = label(screen, "UP/DN BPM  HOLD OK VOL", 285, &lv_font_montserrat_14, TUNER_MUTED);
    label(screen, "HOLD UP: TUNER", 303, &lv_font_montserrat_14, TUNER_MUTED);
    s_metro_screen = screen;
}

static esp_err_t play_click(int beat) {
    int16_t samples[320];
    const float hz = beat == 0 ? 1500.0f : 950.0f;
    for (int i = 0; i < 320; ++i) {
        float decay = (320.0f - i) / 320.0f;
        samples[i] = (int16_t)(14000.0f * decay * sinf(6.2831853f * hz * i / 16000.0f));
    }
    return bsp_audio_write(samples, sizeof(samples));
}

static void show_tuning(void) {
    const tuning_t *tuning = &s_tunings[s_tuning];
    lv_label_set_text_fmt(s_mode, "%s  v", tuning->name);
    int start = (240 - tuning->count * 38) / 2;
    for (int i = 0; i < 6; ++i) {
        if (i < tuning->count) {
            lv_obj_set_x(s_string_tiles[i], start + i * 38);
            lv_label_set_text(s_string_names[i], tuning->labels[i]);
            lv_obj_remove_flag(s_string_tiles[i], LV_OBJ_FLAG_HIDDEN);
        } else lv_obj_add_flag(s_string_tiles[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void show_menu(int selected, bool open) {
    if (open) lv_obj_remove_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < 3; ++i)
        lv_obj_set_style_text_color(s_menu_rows[i],
            lv_color_hex(i == selected ? TUNER_MINT : TUNER_CREAM), 0);
}

static void highlight_string(int midi) {
    for (int i = 0; i < 6; ++i) {
        bool active = i < s_tunings[s_tuning].count && midi == s_tunings[s_tuning].notes[i];
        lv_obj_set_style_border_color(s_string_tiles[i],
                                      lv_color_hex(active ? TUNER_MINT : 0x355477), 0);
        lv_obj_set_style_border_width(s_string_tiles[i], active ? 2 : 1, 0);
    }
}

static void on_key(bsp_btn_t key, bsp_btn_ev_t event, void *arg) {
    (void)arg;
    if (event == BSP_BTN_CLICK || (event == BSP_BTN_LONG && (key == BSP_BTN_OK || key == BSP_BTN_UP))) {
        key_event_t item = {key, event};
        (void)xQueueSend(s_keys, &item, 0);
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(bsp_display_init());
    if (!bsp_lvgl_init() || !bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    build_screen();
    build_metronome_screen();
    bsp_lvgl_unlock();
    bsp_display_backlight(70);
    s_keys = xQueueCreate(8, sizeof(key_event_t));
    int16_t *pcm = malloc(TUNER_FRAME_SAMPLES * sizeof(int16_t));
    if (!s_keys || !pcm) {
        ESP_LOGE(TAG, "Audio/input allocation failed");
        free(pcm);
        if (s_keys) { vQueueDelete(s_keys); s_keys = NULL; }
        if (bsp_lvgl_lock(500)) {
            lv_label_set_text(s_status, "OUT OF MEMORY");
            bsp_lvgl_unlock();
        }
        return;
    }
    esp_err_t audio = bsp_audio_init();
    if (audio == ESP_OK) audio = bsp_audio_set_format(16000, 16, 1);
    if (audio != ESP_OK) {
        ESP_LOGE(TAG, "Microphone failed: %s", esp_err_to_name(audio));
        if (bsp_lvgl_lock(500)) {
            lv_label_set_text(s_status, "MICROPHONE ERROR");
            bsp_lvgl_unlock();
        }
        free(pcm);
        vQueueDelete(s_keys); s_keys = NULL;
        return;
    }
    bsp_audio_set_volume(65);
    bool battery_ok = bsp_battery_init() == ESP_OK;
    esp_err_t buttons = bsp_button_init(on_key, NULL);
    ESP_LOGI(TAG, "TUNER_READY 16k mono, 35..1200 Hz; heap=%u largest=%u buttons=%s",
             (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), esp_err_to_name(buttons));
    int reference = 440;
    bool paused = false, dimmed = false;
    bool menu_open = false;
    int menu_selection = 0;
    bool metro_mode = false;
    metronome_t metro = {.bpm = 120, .volume = 65};
    bool volume_edit = false;
    int metro_beat = -1;
    int64_t last_metro_flash = 0;
    bool metro_audio_error = false;
    int drawn_bpm = -1, drawn_volume = -1, drawn_edit = -1;
    int drawn_metro_status = -1, lit_beat = -1;
    int64_t last_metro_render = 0;
    int prior_note = -1, stable_frames = 0;
    float smoothed_hz = 0;
    int64_t last_signal = esp_timer_get_time(), last_activity = last_signal, last_battery = 0;
    for (;;) {
        key_event_t input;
        while (xQueueReceive(s_keys, &input, 0) == pdTRUE) {
            if (input.key == BSP_BTN_UP && input.event == BSP_BTN_LONG) {
                metro_mode = !metro_mode;
                if (!metro_mode) metronome_stop(&metro);
                menu_open = false;
                prior_note = -1; stable_frames = 0;
                last_signal = esp_timer_get_time() - 1000001;
                if (bsp_lvgl_lock(500)) {
                    lv_screen_load(metro_mode ? s_metro_screen : s_tuner_screen);
                    bsp_lvgl_unlock();
                }
            } else if (metro_mode) {
                if (input.event == BSP_BTN_LONG && input.key == BSP_BTN_OK) {
                    volume_edit = !volume_edit;
                } else if (input.event == BSP_BTN_CLICK && input.key == BSP_BTN_OK) {
                    if (metro.running) metronome_stop(&metro);
                    else { metronome_start(&metro, esp_timer_get_time()); metro_audio_error = false; }
                    metro_beat = -1;
                } else if (input.event == BSP_BTN_CLICK && input.key == BSP_BTN_UP) {
                    if (volume_edit) {
                        metronome_set_volume(&metro, metro.volume + 5);
                        bsp_audio_set_volume(metro.volume);
                    } else metronome_set_bpm(&metro, metro.bpm + 1, esp_timer_get_time());
                } else if (input.event == BSP_BTN_CLICK && input.key == BSP_BTN_DOWN) {
                    if (volume_edit) {
                        metronome_set_volume(&metro, metro.volume - 5);
                        bsp_audio_set_volume(metro.volume);
                    } else metronome_set_bpm(&metro, metro.bpm - 1, esp_timer_get_time());
                }
            } else if (input.key == BSP_BTN_OK && input.event == BSP_BTN_LONG) {
                menu_open = !menu_open;
                menu_selection = s_tuning;
            } else if (menu_open) {
                if (input.key == BSP_BTN_UP && menu_selection > 0) --menu_selection;
                if (input.key == BSP_BTN_DOWN && menu_selection < 2) ++menu_selection;
                if (input.key == BSP_BTN_OK) { s_tuning = menu_selection; menu_open = false; }
            } else {
                if (input.key == BSP_BTN_UP && reference < 450) ++reference;
                if (input.key == BSP_BTN_DOWN && reference > 430) --reference;
                if (input.key == BSP_BTN_OK) paused = !paused;
            }
            prior_note = -1; stable_frames = 0;
            last_activity = esp_timer_get_time();
            if (dimmed) { bsp_display_backlight(70); dimmed = false; }
        }
        if (metro_mode) {
            int64_t now = esp_timer_get_time();
            int beat;
            if (metronome_tick(&metro, now, &beat)) {
                metro_beat = beat;
                last_metro_flash = now;
                if (play_click(beat) != ESP_OK) {
                    ESP_LOGE(TAG, "Metronome audio output failed");
                    metronome_stop(&metro);
                    metro_audio_error = true;
                }
                last_activity = now;
            }
            if (now - last_metro_render < 40000) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            last_metro_render = now;
            bool refresh_battery = now - last_battery > 30000000;
            int soc = -1;
            if (refresh_battery) { soc = battery_ok ? bsp_battery_soc() : -1; last_battery = now; }
            if (!dimmed && now - last_activity > 30000000) {
                bsp_display_backlight(10); dimmed = true;
            }
            if (bsp_lvgl_lock(500)) {
                if (drawn_bpm != metro.bpm) {
                    lv_label_set_text_fmt(s_bpm, "%d", metro.bpm);
                    drawn_bpm = metro.bpm;
                }
                if (drawn_volume != metro.volume) {
                    lv_label_set_text_fmt(s_volume_label, "VOL %d%%", metro.volume);
                    drawn_volume = metro.volume;
                }
                if (drawn_edit != (int)volume_edit) {
                    lv_obj_set_style_text_color(s_bpm,
                        lv_color_hex(volume_edit ? TUNER_CREAM : TUNER_MINT), 0);
                    lv_obj_set_style_text_color(s_volume_label,
                        lv_color_hex(volume_edit ? TUNER_MINT : TUNER_MUTED), 0);
                    lv_label_set_text(s_metro_help, volume_edit ?
                        "UP/DN VOL  HOLD OK BPM" : "UP/DN BPM  HOLD OK VOL");
                    drawn_edit = volume_edit;
                }
                int status = metro_audio_error ? 2 : metro.running ? 1 : 0;
                if (drawn_metro_status != status) {
                    lv_label_set_text(s_metro_status, status == 2 ? "AUDIO ERROR" :
                                      status == 1 ? "OK TO PAUSE" : "OK TO START");
                    drawn_metro_status = status;
                }
                int tip_x = 80;
                if (metro.running) {
                    int64_t period = 60000000LL / metro.bpm;
                    float phase = (float)(metro.next_us - now) / (float)period;
                    if (phase < 0) phase = 0;
                    if (phase > 1) phase = 1;
                    int direction = metro_beat % 2 == 0 ? 1 : -1;
                    tip_x += direction * (int)((0.5f - phase) * 60.0f);
                }
                if ((int)s_pendulum_points[1].x != tip_x) {
                    s_pendulum_points[1].x = tip_x;
                    lv_line_set_points(s_pendulum, s_pendulum_points, 2);
                }
                int new_lit = metro.running && now - last_metro_flash < 120000 ? metro_beat : -1;
                if (lit_beat != new_lit) {
                    if (lit_beat >= 0) lv_obj_set_style_bg_color(s_beats[lit_beat], lv_color_hex(0x355477), 0);
                    if (new_lit >= 0) lv_obj_set_style_bg_color(s_beats[new_lit],
                        lv_color_hex(new_lit == 0 ? TUNER_CORAL : TUNER_MINT), 0);
                    lit_beat = new_lit;
                }
                if (refresh_battery) {
                    if (soc < 0) lv_label_set_text(s_metro_battery, "--%");
                    else lv_label_set_text_fmt(s_metro_battery, "%d%%", soc);
                    if (soc < 0) lv_label_set_text(s_battery, "--%");
                    else lv_label_set_text_fmt(s_battery, "%d%%", soc);
                }
                bsp_lvgl_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        esp_err_t read = bsp_audio_read(pcm, TUNER_FRAME_SAMPLES * sizeof(int16_t));
        if (read != ESP_OK) {
            if (bsp_lvgl_lock(500)) {
                lv_label_set_text(s_status, "MIC READ ERROR");
                bsp_lvgl_unlock();
            }
            ESP_LOGE(TAG, "Microphone read failed");
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        tuner_pitch_t pitch = {0};
        int64_t started = esp_timer_get_time();
        if (!paused) pitch = tuner_detect(pcm, reference);
        int64_t now = esp_timer_get_time();
        int soc = -1;
        bool refresh_battery = now - last_battery > 30000000;
        if (refresh_battery) { soc = battery_ok ? bsp_battery_soc() : -1; last_battery = now; }
        if (pitch.valid) {
            last_signal = last_activity = now;
            if (dimmed) { bsp_display_backlight(70); dimmed = false; }
            if (prior_note == pitch.midi) {
                ++stable_frames;
                smoothed_hz = 0.65f * smoothed_hz + 0.35f * pitch.frequency;
            } else {
                prior_note = pitch.midi; stable_frames = 1;
                smoothed_hz = pitch.frequency;
            }
            pitch.cents = 1200 * log2f(smoothed_hz / reference) - 100 * (pitch.midi - 69);
        }
        if (!dimmed && now - last_activity > 30000000) {
            bsp_display_backlight(10); dimmed = true;
        }
        if (!bsp_lvgl_lock(500)) continue;
        lv_label_set_text_fmt(s_reference, "A4 %d Hz  |  HOLD OK: MODE", reference);
        show_tuning();
        show_menu(menu_selection, menu_open);
        if (refresh_battery) {
            if (soc < 0) lv_label_set_text(s_battery, "--%");
            else lv_label_set_text_fmt(s_battery, "%d%%", soc);
            if (soc < 0) lv_label_set_text(s_metro_battery, "--%");
            else lv_label_set_text_fmt(s_metro_battery, "%d%%", soc);
        }
        if (paused || now - last_signal > 1000000) {
            lv_label_set_text(s_note, "--");
            lv_label_set_text(s_frequency, "-- Hz");
            lv_label_set_text(s_cents, "-- cents");
            lv_label_set_text(s_status, paused ? "PAUSED" : "PLUCK ONE STRING");
            lv_obj_add_flag(s_needle, LV_OBJ_FLAG_HIDDEN);
            highlight_string(-1);
            prior_note = -1; stable_frames = 0;
        } else if (pitch.valid && stable_frames >= 2) {
            lv_label_set_text_fmt(s_note, "%s%d", tuner_note_name(pitch.midi), pitch.midi / 12 - 1);
            lv_label_set_text_fmt(s_frequency, "%.1f Hz", smoothed_hz);
            lv_label_set_text_fmt(s_cents, "%+.0f cents", pitch.cents);
            bool in_tune = fabsf(pitch.cents) <= 5;
            uint32_t color = in_tune ? TUNER_MINT : pitch.cents < 0 ? TUNER_CORAL : TUNER_AMBER;
            lv_label_set_text(s_status, in_tune ? "IN TUNE" : pitch.cents < 0 ? "A LITTLE LOW" : "A LITTLE HIGH");
            lv_obj_set_style_text_color(s_cents, lv_color_hex(color), 0);
            lv_obj_set_style_line_color(s_needle, lv_color_hex(color), 0);
            float cents = fmaxf(-50, fminf(50, pitch.cents));
            int tip_x = 80 + (int)(cents * 0.6f);
            if ((int)s_tuner_needle_points[1].x != tip_x) {
                s_tuner_needle_points[1].x = tip_x;
                lv_line_set_points(s_needle, s_tuner_needle_points, 2);
            }
            lv_obj_remove_flag(s_needle, LV_OBJ_FLAG_HIDDEN);
            highlight_string(pitch.midi);
        }
        bsp_lvgl_unlock();
        static unsigned log_frame;
        if (++log_frame % 40 == 0) ESP_LOGI(TAG, "pitch=%d hz=%.1f rms=%.0f analysis_us=%lld", pitch.valid, pitch.frequency, pitch.rms, (long long)(now - started));
        // Yield even if I2S already buffered the next frame.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
