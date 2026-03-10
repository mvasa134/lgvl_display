// wifi_screen.cpp
// Изменения для ESPHome-хоста:
//   - WiFi.disconnect() / WiFi.begin() НЕЛЬЗЯ вызывать напрямую —
//     ESPHome держит wifi-стек под своим управлением.
//   - Подключение к новой сети делается через ESPHome wifi компонент:
//     используем esp_wifi_* напрямую (IDF-уровень) либо
//     стандартный Arduino WiFi — он работает поверх IDF и совместим
//     с ESPHome при условии, что мы НЕ дёргаем WiFi.mode() и
//     НЕ вызываем WiFi.disconnect(true) (это сбрасывает NVS и
//     ломает ESPHome reconnect).
//   - WiFi.begin(ssid, pass) — допустимо, ESPHome подхватит новые
//     credentials при следующем реконнекте.
//   - Сканирование сетей (WiFi.scanNetworks) — полностью совместимо.

#include "wifi_screen.h"
#include "ui.h"
#include <WiFi.h>

#define UI_W 1280
#define UI_H 800

static lv_obj_t *layer_wifi     = nullptr;
static lv_obj_t *wifi_list      = nullptr;
static lv_obj_t *lbl_wifi_status = nullptr;
static lv_obj_t *panel_password = nullptr;
static lv_obj_t *ta_password    = nullptr;
static lv_obj_t *kb             = nullptr;
static lv_obj_t *lbl_cur_ssid  = nullptr;
static lv_obj_t *lbl_cur_ip    = nullptr;
static char selected_ssid[64]  = "";

static void do_scan();
static void on_network_click(lv_event_t *e);
static void on_connect_click(lv_event_t *e);
static void on_back_click(lv_event_t *e);
static void on_kb_event(lv_event_t *e);

// ── Публичные функции ─────────────────────────────────────────────────────

void show_wifi_screen() {
    lv_obj_clear_flag(layer_wifi, LV_OBJ_FLAG_HIDDEN);
    do_scan();
}

void hide_wifi_screen() {
    lv_obj_add_flag(layer_wifi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(panel_password, LV_OBJ_FLAG_HIDDEN);
}

// ── Обновление инфо-панели каждую секунду ────────────────────────────────

static void update_info_cb(lv_timer_t *t) {
    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(lbl_cur_ssid,
            (String(LV_SYMBOL_WIFI " SSID: ") + WiFi.SSID()).c_str());
        lv_label_set_text(lbl_cur_ip,
            (String(LV_SYMBOL_DIRECTORY " IP: ") + WiFi.localIP().toString()).c_str());
    } else {
        lv_label_set_text(lbl_cur_ssid, "Not connected");
        lv_label_set_text(lbl_cur_ip, "");
    }
}

// ── Сканирование ──────────────────────────────────────────────────────────

static lv_timer_t *scan_timer = nullptr;

static void scan_check_cb(lv_timer_t *timer) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return;

    if (n == WIFI_SCAN_FAILED || n == 0) {
        lv_label_set_text(lbl_wifi_status, "No networks found");
        lv_timer_delete(timer);
        scan_timer = nullptr;
        return;
    }

    char status_buf[32];
    snprintf(status_buf, sizeof(status_buf), "Found %d networks", n);
    lv_label_set_text(lbl_wifi_status, status_buf);
    lv_obj_clean(wifi_list);

    for (int i = 0; i < n && i < 12; i++) {
        lv_obj_t *row = lv_obj_create(wifi_list);
        lv_obj_set_size(row, 560, 52);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x0D0D18), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x222233), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *lbl_ssid = lv_label_create(row);
        lv_label_set_text(lbl_ssid, WiFi.SSID(i).c_str());
        lv_obj_set_style_text_color(lbl_ssid, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ssid, LV_ALIGN_LEFT_MID, 16, 0);

        int rssi = WiFi.RSSI(i);
        char rssi_buf[16];
        snprintf(rssi_buf, sizeof(rssi_buf), "%d dBm", rssi);
        lv_obj_t *lbl_rssi = lv_label_create(row);
        lv_label_set_text(lbl_rssi, rssi_buf);
        lv_obj_set_style_text_color(lbl_rssi,
            rssi > -60 ? lv_color_hex(0x00FF88) :
            rssi > -75 ? lv_color_hex(0xFFCC00) : lv_color_hex(0xFF3060), 0);
        lv_obj_set_style_text_font(lbl_rssi, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_rssi, LV_ALIGN_RIGHT_MID, -16, 0);

        lv_obj_set_user_data(row, (void*)lv_label_get_text(lbl_ssid));
        lv_obj_add_event_cb(row, on_network_click, LV_EVENT_CLICKED, NULL);
    }

    WiFi.scanDelete();
    lv_timer_delete(timer);
    scan_timer = nullptr;
}

static void do_scan() {
    if (scan_timer) return;
    // НЕ вызываем WiFi.disconnect() — это ломает ESPHome wifi менеджер.
    // Сканирование работает параллельно с активным подключением (passive scan).
    lv_label_set_text(lbl_wifi_status, "Scanning... Please wait");
    lv_obj_clean(wifi_list);

    // async=true, show_hidden=false, passive=false, max_ms_per_chan=300
    int16_t n = WiFi.scanNetworks(/*async=*/true, false, false, 300);
    if (n == WIFI_SCAN_FAILED) {
        lv_label_set_text(lbl_wifi_status, "Scan failed to start");
        return;
    }
    scan_timer = lv_timer_create(scan_check_cb, 500, NULL);
}

// ── Клик на сеть ─────────────────────────────────────────────────────────

static void on_network_click(lv_event_t *e) {
    lv_obj_t *row = (lv_obj_t*)lv_event_get_target(e);
    const char *ssid = (const char*)lv_obj_get_user_data(row);
    strncpy(selected_ssid, ssid, sizeof(selected_ssid) - 1);

    lv_obj_t *lbl_panel_title = (lv_obj_t*)lv_obj_get_child(panel_password, 0);
    char buf[80];
    snprintf(buf, sizeof(buf), "Connect to: %s", selected_ssid);
    lv_label_set_text(lbl_panel_title, buf);

    lv_textarea_set_text(ta_password, "");
    lv_obj_clear_flag(panel_password, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(kb, ta_password);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

// ── Подключение ───────────────────────────────────────────────────────────

static void on_connect_click(lv_event_t *e) {
    const char *pass = lv_textarea_get_text(ta_password);
    Serial.printf("[WiFi] Подключаемся к SSID: '%s'\n", selected_ssid);

    lv_label_set_text(lbl_wifi_status, "Connecting...");
    lv_obj_add_flag(panel_password, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    // ВАЖНО для ESPHome:
    //   WiFi.begin() здесь — мягкий запрос переподключения к новой сети.
    //   НЕ используем WiFi.disconnect(true) — это сотрёт NVS credentials
    //   ESPHome и сломает автоматическое переподключение.
    //   НЕ используем delay() внутри LVGL-таймеров.
    WiFi.begin(selected_ssid, pass);

    lv_timer_create([](lv_timer_t *timer) {
        static int attempts = 0;
        attempts++;
        Serial.printf("[WiFi] status=%d attempt=%d\n", WiFi.status(), attempts);

        if (WiFi.status() == WL_CONNECTED) {
            String msg = String("Connected: ") + WiFi.localIP().toString();
            lv_label_set_text(lbl_wifi_status, msg.c_str());
            lv_timer_delete(timer);
            attempts = 0;
        } else if (attempts > 30) {   // 30 × 500ms = 15 сек таймаут
            lv_label_set_text(lbl_wifi_status, "Connection failed");
            lv_timer_delete(timer);
            attempts = 0;
        }
    }, 500, NULL);
}

// ── Кнопка "Назад" ────────────────────────────────────────────────────────

static void on_back_click(lv_event_t *e) {
    hide_wifi_screen();
}

// ── Клавиатура ────────────────────────────────────────────────────────────

static void on_kb_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)        on_connect_click(e);
    else if (code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(panel_password, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── Построение экрана ─────────────────────────────────────────────────────

void build_wifi_screen() {
    lv_obj_t *scr = lv_screen_active();

    // Таймер обновления инфо-панели
    lv_timer_create(update_info_cb, 1000, NULL);

    layer_wifi = lv_obj_create(scr);
    lv_obj_set_size(layer_wifi, UI_W, UI_H);
    lv_obj_align(layer_wifi, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(layer_wifi, lv_color_hex(0x05050F), 0);
    lv_obj_set_style_bg_opa(layer_wifi, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(layer_wifi, 0, 0);
    lv_obj_set_style_pad_all(layer_wifi, 0, 0);
    lv_obj_clear_flag(layer_wifi, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(layer_wifi, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(layer_wifi, LV_OBJ_FLAG_HIDDEN);

    // Топбар
    lv_obj_t *topbar = lv_obj_create(layer_wifi);
    lv_obj_set_size(topbar, UI_W, 48);
    lv_obj_align(topbar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(0x0A1020), 0);
    lv_obj_set_style_border_width(topbar, 1, 0);
    lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(topbar, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_pad_all(topbar, 0, 0);
    lv_obj_clear_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(topbar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *lbl_title = lv_label_create(topbar);
    lv_label_set_text(lbl_title, "WIFI SETTINGS");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_back = lv_btn_create(topbar);
    lv_obj_set_size(btn_back, 80, 34);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_add_event_cb(btn_back, on_back_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_back);

    lv_obj_t *btn_rescan = lv_btn_create(topbar);
    lv_obj_set_size(btn_rescan, 100, 34);
    lv_obj_align(btn_rescan, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btn_rescan, lv_color_hex(0x0A1628), 0);
    lv_obj_set_style_border_color(btn_rescan, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_border_width(btn_rescan, 1, 0);
    lv_obj_set_style_radius(btn_rescan, 4, 0);
    lv_obj_add_event_cb(btn_rescan, [](lv_event_t*){ do_scan(); },
                        LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_rescan = lv_label_create(btn_rescan);
    lv_label_set_text(lbl_rescan, LV_SYMBOL_REFRESH " Scan");
    lv_obj_set_style_text_color(lbl_rescan, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl_rescan, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_rescan);

    // Статус
    lbl_wifi_status = lv_label_create(layer_wifi);
    lv_label_set_text(lbl_wifi_status, "Press Scan to find networks");
    lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(0x4A7090), 0);
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_wifi_status, LV_ALIGN_TOP_LEFT, 20, 60);

    // Список сетей
    wifi_list = lv_obj_create(layer_wifi);
    lv_obj_set_size(wifi_list, 600, 680);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_LEFT, 20, 90);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x0A0A12), 0);
    lv_obj_set_style_bg_opa(wifi_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wifi_list, 1, 0);
    lv_obj_set_style_border_color(wifi_list, lv_color_hex(0x222233), 0);
    lv_obj_set_style_radius(wifi_list, 12, 0);
    lv_obj_set_style_pad_all(wifi_list, 10, 0);
    lv_obj_set_flex_flow(wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(wifi_list, 6, 0);

    // Карточка текущего подключения
    lv_obj_t *card_status = lv_obj_create(layer_wifi);
    lv_obj_set_size(card_status, 600, 200);
    lv_obj_align(card_status, LV_ALIGN_TOP_RIGHT, -20, 90);
    lv_obj_set_style_bg_color(card_status, lv_color_hex(0x0D0D18), 0);
    lv_obj_set_style_border_width(card_status, 1, 0);
    lv_obj_set_style_border_color(card_status, lv_color_hex(0x222233), 0);
    lv_obj_set_style_radius(card_status, 12, 0);
    lv_obj_set_style_pad_all(card_status, 16, 0);
    lv_obj_clear_flag(card_status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(card_status, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *lbl_cur_title = lv_label_create(card_status);
    lv_label_set_text(lbl_cur_title, "CURRENT CONNECTION");
    lv_obj_set_style_text_color(lbl_cur_title, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(lbl_cur_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cur_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_cur_ssid = lv_label_create(card_status);
    lv_obj_set_style_text_font(lbl_cur_ssid, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cur_ssid, LV_ALIGN_TOP_LEFT, 0, 30);

    lbl_cur_ip = lv_label_create(card_status);
    lv_obj_set_style_text_font(lbl_cur_ip, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cur_ip, LV_ALIGN_TOP_LEFT, 0, 60);

    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(lbl_cur_ssid,
            (String("SSID: ") + WiFi.SSID()).c_str());
        lv_obj_set_style_text_color(lbl_cur_ssid, lv_color_hex(0x00FF88), 0);
        lv_label_set_text(lbl_cur_ip,
            (String("IP: ") + WiFi.localIP().toString()).c_str());
        lv_obj_set_style_text_color(lbl_cur_ip, lv_color_hex(0x00B4FF), 0);
    } else {
        lv_label_set_text(lbl_cur_ssid, "Not connected");
        lv_obj_set_style_text_color(lbl_cur_ssid, lv_color_hex(0xFF3060), 0);
        lv_label_set_text(lbl_cur_ip, "");
    }

    // Панель пароля
    panel_password = lv_obj_create(layer_wifi);
    lv_obj_set_size(panel_password, 600, 160);
    lv_obj_align(panel_password, LV_ALIGN_TOP_RIGHT, -20, 310);
    lv_obj_set_style_bg_color(panel_password, lv_color_hex(0x0D0D18), 0);
    lv_obj_set_style_border_width(panel_password, 1, 0);
    lv_obj_set_style_border_color(panel_password, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_radius(panel_password, 12, 0);
    lv_obj_set_style_pad_all(panel_password, 16, 0);
    lv_obj_clear_flag(panel_password, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(panel_password, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(panel_password, LV_OBJ_FLAG_HIDDEN);

    // child[0] — используется в on_network_click для обновления заголовка
    lv_obj_t *lbl_panel_title = lv_label_create(panel_password);
    lv_label_set_text(lbl_panel_title, "Connect to: ...");
    lv_obj_set_style_text_color(lbl_panel_title, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(lbl_panel_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_panel_title, LV_ALIGN_TOP_LEFT, 0, 0);

    ta_password = lv_textarea_create(panel_password);
    lv_obj_set_size(ta_password, 420, 44);
    lv_obj_align(ta_password, LV_ALIGN_TOP_LEFT, 0, 36);
    lv_textarea_set_password_mode(ta_password, true);
    lv_textarea_set_one_line(ta_password, true);
    lv_textarea_set_placeholder_text(ta_password, "Password...");
    lv_obj_set_style_bg_color(ta_password, lv_color_hex(0x0A1020), 0);
    lv_obj_set_style_text_color(ta_password, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(ta_password, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_text_font(ta_password, &lv_font_montserrat_14, 0);

    lv_obj_t *btn_connect = lv_btn_create(panel_password);
    lv_obj_set_size(btn_connect, 140, 44);
    lv_obj_align(btn_connect, LV_ALIGN_TOP_RIGHT, 0, 36);
    lv_obj_set_style_bg_color(btn_connect, lv_color_hex(0x003A60), 0);
    lv_obj_set_style_border_color(btn_connect, lv_color_hex(0x00B4FF), 0);
    lv_obj_set_style_border_width(btn_connect, 1, 0);
    lv_obj_set_style_radius(btn_connect, 4, 0);
    lv_obj_add_event_cb(btn_connect, on_connect_click, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_conn = lv_label_create(btn_connect);
    lv_label_set_text(lbl_conn, LV_SYMBOL_WIFI " Connect");
    lv_obj_set_style_text_color(lbl_conn, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(lbl_conn, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_conn);

    // Клавиатура
    kb = lv_keyboard_create(layer_wifi);
    lv_obj_set_size(kb, UI_W, 300);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta_password);
    lv_obj_add_event_cb(kb, on_kb_event, LV_EVENT_READY,  NULL);
    lv_obj_add_event_cb(kb, on_kb_event, LV_EVENT_CANCEL, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}
