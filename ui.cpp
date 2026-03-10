// ui.cpp
#include "ui.h"
#include "wifi_screen.h"
#include <Arduino.h>
#include <time.h>
// Логические размеры после поворота
#define UI_W 1280
#define UI_H 800
lv_obj_t *layer_loading;
lv_obj_t *layer_main;
lv_obj_t *loading_bar;
lv_obj_t *ha_message_label;
static lv_obj_t *lbl_clock = nullptr;
lv_obj_t *create_card(lv_obj_t *parent, int w, int h) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A1A24), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x333344), 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}
static void update_clock_cb(lv_timer_t * timer) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Проверка: если год меньше 2000, значит время еще не синхронизировано
    if (timeinfo.tm_year < 100) { 
        lv_label_set_text(lbl_clock, "Syncing...");
        return;
    }

    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    lv_label_set_text(lbl_clock, buf);
}
void loading_timer_cb(lv_timer_t *timer) {
    static int progress = 0;
    progress += 2;
    lv_bar_set_value(loading_bar, progress, LV_ANIM_ON);
    if (progress >= 100) {
        lv_timer_delete(timer);
        lv_obj_add_flag(layer_loading, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(layer_main, LV_OBJ_FLAG_HIDDEN);
    }
}

void build_main_screen() {
    lv_obj_t *scr = lv_screen_active();

    layer_main = lv_obj_create(scr);
    lv_obj_set_size(layer_main, UI_W, UI_H);
    lv_obj_align(layer_main, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(layer_main, lv_color_hex(0x0D0D14), 0);
    lv_obj_set_style_bg_opa(layer_main, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(layer_main, 0, 0);
    lv_obj_set_style_pad_all(layer_main, 0, 0);
    lv_obj_clear_flag(layer_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(layer_main, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(layer_main, LV_OBJ_FLAG_HIDDEN);

    // ── ТОПБАР ─────────────────────────────────────────────────
    lv_obj_t *topbar = lv_obj_create(layer_main);
    lv_obj_set_size(topbar, UI_W, 48);
    lv_obj_align(topbar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(0x0A1020), 0);
    lv_obj_set_style_border_width(topbar, 0, 0);
    lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(topbar, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(topbar, 1, 0);
    lv_obj_set_style_pad_all(topbar, 0, 0);
    lv_obj_clear_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(topbar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *lbl_title = lv_label_create(topbar);
    lv_label_set_text(lbl_title, "HOME OS");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 20, 0);

    // Убираем lv_obj_t *, чтобы использовать статическую переменную
lbl_clock = lv_label_create(topbar); 
lv_label_set_text(lbl_clock, "00:00:00");
lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0x00B4FF), 0);
lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_14, 0);
lv_obj_align(lbl_clock, LV_ALIGN_CENTER, 0, 0);

// Запускаем таймер обновления раз в секунду
lv_timer_create(update_clock_cb, 1000, NULL);

    lv_obj_t *lbl_status = lv_label_create(topbar);
    lv_label_set_text(lbl_status, "● HA ONLINE");
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_status, LV_ALIGN_RIGHT_MID, -20, 0);

    // ── В конце build_main_screen(), после lbl_status ──────────
lv_obj_t *btn_settings = lv_btn_create(topbar);
lv_obj_set_size(btn_settings, 40, 34);
lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -130, 0);
lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x0A1020), 0);
lv_obj_set_style_border_width(btn_settings, 0, 0);
lv_obj_add_event_cb(btn_settings, [](lv_event_t*){ show_wifi_screen(); },
                    LV_EVENT_CLICKED, NULL);
lv_obj_t *lbl_set = lv_label_create(btn_settings);
lv_label_set_text(lbl_set, LV_SYMBOL_SETTINGS);
lv_obj_set_style_text_color(lbl_set, lv_color_hex(0x4A7090), 0);
lv_obj_set_style_text_font(lbl_set, &lv_font_montserrat_14, 0);
lv_obj_center(lbl_set);
    // ── СЕТКА: 3 колонки, отступ сверху 58px ──────────────────
    // Колонка 1: x=10,    w=370  — HA Messages
    // Колонка 2: x=390,   w=490  — Thermostat
    // Колонка 3: x=890,   w=380  — Media + Jarvis
    // Высота рабочей зоны: 800-58-10 = 732px

    // ── КОЛОНКА 1: HA MESSAGES ─────────────────────────────────
    lv_obj_t *card_ha = create_card(layer_main, 370, 732);
    lv_obj_align(card_ha, LV_ALIGN_TOP_LEFT, 10, 58);

    lv_obj_t *ha_title = lv_label_create(card_ha);
    lv_label_set_text(ha_title, "HOME ASSISTANT");
    lv_obj_set_style_text_color(ha_title, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(ha_title, &lv_font_montserrat_14, 0);
    lv_obj_align(ha_title, LV_ALIGN_TOP_LEFT, 12, 12);

    // Прокручиваемый список сообщений
    lv_obj_t *msg_list = lv_obj_create(card_ha);
    lv_obj_set_size(msg_list, 346, 680);
    lv_obj_align(msg_list, LV_ALIGN_TOP_LEFT, 0, 36);
    lv_obj_set_style_bg_opa(msg_list, LV_OPA_0, 0);
    lv_obj_set_style_border_width(msg_list, 0, 0);
    lv_obj_set_style_pad_all(msg_list, 8, 0);
    lv_obj_set_flex_flow(msg_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(msg_list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(msg_list, LV_SCROLLBAR_MODE_OFF);

    // Вспомогательная структура для сообщений
    struct { const char *tag; const char *text; uint32_t color; } msgs[] = {
        { "[14:32] ALERT",  "Motion detected at front door. Zone: Porch.",      0xFF3060 },
        { "[14:18] WARN",   "Energy > 3.2 kWh. Peak tariff active.",            0xFFCC00 },
        { "[13:55] OK",     "Front door locked automatically.",                 0x00FF88 },
        { "[13:00] INFO",   "Evening scene ON. Lights set to Sunset mode.",     0x00B4FF },
        { "[12:30] INFO",   "Boiler switched to eco mode. Temp set to 18C.",    0x4A7090 },
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *row = lv_obj_create(msg_list);
        lv_obj_set_size(row, 330, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x0D0D18), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(msgs[i].color), 0);
        lv_obj_set_style_border_width(row, 2, 0);
        lv_obj_set_style_pad_left(row, 10, 0);
        lv_obj_set_style_pad_top(row, 6, 0);
        lv_obj_set_style_pad_bottom(row, 6, 0);
        lv_obj_set_style_pad_right(row, 6, 0);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_set_style_margin_bottom(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);

        lv_obj_t *lbl_tag = lv_label_create(row);
        lv_label_set_text(lbl_tag, msgs[i].tag);
        lv_obj_set_style_text_color(lbl_tag, lv_color_hex(msgs[i].color), 0);
        lv_obj_set_style_text_font(lbl_tag, &lv_font_montserrat_14, 0);

        lv_obj_t *lbl_msg = lv_label_create(row);
        lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_msg, 310);
        lv_label_set_text(lbl_msg, msgs[i].text);
        lv_obj_set_style_text_color(lbl_msg, lv_color_hex(0xCCCCDD), 0);
        lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_14, 0);
    }

    // ── КОЛОНКА 2: THERMOSTAT ──────────────────────────────────
    lv_obj_t *card_climate = create_card(layer_main, 490, 732);
    lv_obj_align(card_climate, LV_ALIGN_TOP_LEFT, 390, 58);

    lv_obj_t *climate_title = lv_label_create(card_climate);
    lv_label_set_text(climate_title, "CLIMATE CONTROL");
    lv_obj_set_style_text_color(climate_title, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(climate_title, &lv_font_montserrat_14, 0);
    lv_obj_align(climate_title, LV_ALIGN_TOP_LEFT, 12, 12);

    lv_obj_t *arc = lv_arc_create(card_climate);
    lv_obj_set_size(arc, 300, 300);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 14, 32);
    lv_arc_set_value(arc, 22);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1A2030), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xFF6B35), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_0, LV_PART_MAIN);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *temp_val = lv_label_create(card_climate);
    lv_label_set_text(temp_val, "22");
    lv_obj_set_style_text_color(temp_val, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(temp_val, &lv_font_montserrat_14, 0);
    lv_obj_align(temp_val, LV_ALIGN_TOP_MID, 0, 165);

    lv_obj_t *temp_unit = lv_label_create(card_climate);
    lv_label_set_text(temp_unit, "\xC2\xB0""C");
    lv_obj_set_style_text_color(temp_unit, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(temp_unit, LV_ALIGN_TOP_MID, 36, 175);

    // Статы
    struct { const char *name; const char *val; uint32_t col; } stats[] = {
        { "ACTUAL",   "19.4 C", 0x00B4FF },
        { "HUMIDITY", "47%",    0x00FFCC },
        { "OUTDOOR",  "7.2 C",  0x4A7090 },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_obj_create(card_climate);
        lv_obj_set_size(row, 440, 38);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 20, 360 + i * 50);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x0D0D18), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x222233), 0);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *lbl_n = lv_label_create(row);
        lv_label_set_text(lbl_n, stats[i].name);
        lv_obj_set_style_text_color(lbl_n, lv_color_hex(0x4A7090), 0);
        lv_obj_set_style_text_font(lbl_n, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_n, LV_ALIGN_LEFT_MID, 12, 0);

        lv_obj_t *lbl_v = lv_label_create(row);
        lv_label_set_text(lbl_v, stats[i].val);
        lv_obj_set_style_text_color(lbl_v, lv_color_hex(stats[i].col), 0);
        lv_obj_set_style_text_font(lbl_v, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_v, LV_ALIGN_RIGHT_MID, -12, 0);
    }

    // Кнопки + / -
    lv_obj_t *btn_minus = lv_btn_create(card_climate);
    lv_obj_set_size(btn_minus, 80, 40);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_minus, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(btn_minus, 1, 0);
    lv_obj_set_style_radius(btn_minus, 4, 0);
    lv_obj_align(btn_minus, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_obj_t *lbl_minus = lv_label_create(btn_minus);
    lv_label_set_text(lbl_minus, "-");
    lv_obj_set_style_text_color(lbl_minus, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(lbl_minus, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_minus);

    lv_obj_t *btn_plus = lv_btn_create(card_climate);
    lv_obj_set_size(btn_plus, 80, 40);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_plus, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_border_width(btn_plus, 1, 0);
    lv_obj_set_style_radius(btn_plus, 4, 0);
    lv_obj_align(btn_plus, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    lv_obj_t *lbl_plus = lv_label_create(btn_plus);
    lv_label_set_text(lbl_plus, "+");
    lv_obj_set_style_text_color(lbl_plus, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_text_font(lbl_plus, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_plus);

    // ── КОЛОНКА 3: MEDIA (верх) + JARVIS (низ) ────────────────
    // MEDIA
    lv_obj_t *card_media = create_card(layer_main, 370, 340);
    lv_obj_align(card_media, LV_ALIGN_TOP_LEFT, 890, 58);

    lv_obj_t *media_hdr = lv_label_create(card_media);
    lv_label_set_text(media_hdr, "MEDIA CONTROL");
    lv_obj_set_style_text_color(media_hdr, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(media_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(media_hdr, LV_ALIGN_TOP_LEFT, 12, 12);

    lv_obj_t *track_name = lv_label_create(card_media);
    lv_label_set_long_mode(track_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(track_name, 330);
    lv_label_set_text(track_name, "Interstellar Suite — Hans Zimmer");
    lv_obj_set_style_text_color(track_name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(track_name, &lv_font_montserrat_14, 0);
    lv_obj_align(track_name, LV_ALIGN_TOP_MID, 0, 38);

    lv_obj_t *track_artist = lv_label_create(card_media);
    lv_label_set_text(track_artist, "Sci-Fi Ambient");
    lv_obj_set_style_text_color(track_artist, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(track_artist, &lv_font_montserrat_14, 0);
    lv_obj_align(track_artist, LV_ALIGN_TOP_MID, 0, 62);

    // Прогресс бар
    lv_obj_t *bar_prog = lv_bar_create(card_media);
    lv_obj_set_size(bar_prog, 330, 4);
    lv_bar_set_range(bar_prog, 0, 100);
    lv_bar_set_value(bar_prog, 35, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_prog, lv_color_hex(0x0A2040), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_prog, lv_color_hex(0x00B4FF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_prog, 2, 0);
    lv_obj_set_style_radius(bar_prog, 2, LV_PART_INDICATOR);
    lv_obj_align(bar_prog, LV_ALIGN_TOP_MID, 0, 88);

    lv_obj_t *lbl_t1 = lv_label_create(card_media);
    lv_label_set_text(lbl_t1, "1:24");
    lv_obj_set_style_text_color(lbl_t1, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_t1, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_t1, LV_ALIGN_TOP_LEFT, 18, 98);

    lv_obj_t *lbl_t2 = lv_label_create(card_media);
    lv_label_set_text(lbl_t2, "4:02");
    lv_obj_set_style_text_color(lbl_t2, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_t2, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_t2, LV_ALIGN_TOP_RIGHT, -18, 98);

    // Кнопки Prev / Play / Next
    lv_obj_t *btn_prev = lv_btn_create(card_media);
    lv_obj_set_size(btn_prev, 60, 50);
    lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_prev, lv_color_hex(0x222233), 0);
    lv_obj_set_style_border_width(btn_prev, 1, 0);
    lv_obj_set_style_radius(btn_prev, 4, 0);
    lv_obj_align(btn_prev, LV_ALIGN_TOP_MID, -78, 120);
    lv_obj_t *lbl_prev = lv_label_create(btn_prev);
    lv_label_set_text(lbl_prev, "|<");
    lv_obj_set_style_text_color(lbl_prev, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_prev, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_prev);

    lv_obj_t *btn_play = lv_btn_create(card_media);
    lv_obj_set_size(btn_play, 72, 58);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0x001A30), 0);
    lv_obj_set_style_border_color(btn_play, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(btn_play, 1, 0);
    lv_obj_set_style_radius(btn_play, 4, 0);
    lv_obj_set_style_shadow_color(btn_play, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_shadow_width(btn_play, 12, 0);
    lv_obj_set_style_shadow_opa(btn_play, LV_OPA_40, 0);
    lv_obj_align(btn_play, LV_ALIGN_TOP_MID, 0, 116);
    lv_obj_t *lbl_play = lv_label_create(btn_play);
    lv_label_set_text(lbl_play, "||");
    lv_obj_set_style_text_color(lbl_play, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(lbl_play, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_play);

    lv_obj_t *btn_next = lv_btn_create(card_media);
    lv_obj_set_size(btn_next, 60, 50);
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_next, lv_color_hex(0x222233), 0);
    lv_obj_set_style_border_width(btn_next, 1, 0);
    lv_obj_set_style_radius(btn_next, 4, 0);
    lv_obj_align(btn_next, LV_ALIGN_TOP_MID, 78, 120);
    lv_obj_t *lbl_next = lv_label_create(btn_next);
    lv_label_set_text(lbl_next, ">|");
    lv_obj_set_style_text_color(lbl_next, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_next, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_next);

    // Громкость
    lv_obj_t *lbl_vol = lv_label_create(card_media);
    lv_label_set_text(lbl_vol, "VOL");
    lv_obj_set_style_text_color(lbl_vol, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_vol, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_vol, LV_ALIGN_BOTTOM_LEFT, 18, -28);

    lv_obj_t *slider_vol = lv_slider_create(card_media);
    lv_obj_set_size(slider_vol, 280, 6);
    lv_slider_set_value(slider_vol, 70, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_vol, lv_color_hex(0x0A2040), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_vol, lv_color_hex(0x00FFCC), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_vol, lv_color_hex(0x00FFCC), LV_PART_KNOB);
    lv_obj_set_style_radius(slider_vol, 3, 0);
    lv_obj_align(slider_vol, LV_ALIGN_BOTTOM_RIGHT, -18, -30);

    // JARVIS
    lv_obj_t *card_jarvis = create_card(layer_main, 370, 380);
    lv_obj_align(card_jarvis, LV_ALIGN_TOP_LEFT, 890, 408);

    lv_obj_t *jarvis_hdr = lv_label_create(card_jarvis);
    lv_label_set_text(jarvis_hdr, "JARVIS AI");
    lv_obj_set_style_text_color(jarvis_hdr, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(jarvis_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(jarvis_hdr, LV_ALIGN_TOP_LEFT, 12, 12);

    lv_obj_t *btn_jarvis = lv_btn_create(card_jarvis);
    lv_obj_set_size(btn_jarvis, 160, 160);
    lv_obj_set_style_radius(btn_jarvis, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(btn_jarvis, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(btn_jarvis, lv_color_hex(0x001A30), 0);
    lv_obj_set_style_border_color(btn_jarvis, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(btn_jarvis, 2, 0);
    lv_obj_set_style_shadow_color(btn_jarvis, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_shadow_width(btn_jarvis, 40, 0);
    lv_obj_set_style_shadow_opa(btn_jarvis, LV_OPA_60, 0);

    lv_obj_t *lbl_j1 = lv_label_create(btn_jarvis);
    lv_label_set_text(lbl_j1, "JARVIS");
    lv_obj_set_style_text_color(lbl_j1, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(lbl_j1, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_j1, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *lbl_j2 = lv_label_create(btn_jarvis);
    lv_label_set_text(lbl_j2, "STANDBY");
    lv_obj_set_style_text_color(lbl_j2, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_j2, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_j2, LV_ALIGN_CENTER, 0, 12);
}

void build_loading_screen() {
    lv_obj_t *scr = lv_screen_active();

    layer_loading = lv_obj_create(scr);
    lv_obj_set_size(layer_loading, UI_W, UI_H);
    lv_obj_align(layer_loading, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(layer_loading, lv_color_hex(0x05050A), 0);
    lv_obj_set_style_bg_opa(layer_loading, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(layer_loading, 0, 0);
    lv_obj_set_style_pad_all(layer_loading, 0, 0);
    lv_obj_clear_flag(layer_loading, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(layer_loading, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *logo = lv_label_create(layer_loading);
    lv_label_set_text(logo, "J.A.R.V.I.S.\nSYSTEM INITIALIZATION...");
    lv_obj_set_style_text_align(logo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(logo, lv_color_hex(0x0088FF), 0);
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_14, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -50);

    loading_bar = lv_bar_create(layer_loading);
    lv_obj_set_size(loading_bar, 600, 12);
    lv_obj_align(loading_bar, LV_ALIGN_CENTER, 0, 50);
    lv_bar_set_range(loading_bar, 0, 100);
    lv_bar_set_value(loading_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(loading_bar, lv_color_hex(0x0088FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(loading_bar, lv_color_hex(0x111122), LV_PART_MAIN);
    lv_obj_set_style_radius(loading_bar, 4, 0);
    lv_obj_set_style_radius(loading_bar, 4, LV_PART_INDICATOR);
}

void ui_init(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    build_main_screen();
    build_loading_screen();
     build_wifi_screen();
    lv_timer_create(loading_timer_cb, 30, NULL);
}
