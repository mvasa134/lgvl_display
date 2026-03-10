#pragma once
#include <lvgl.h>

// Публичные объекты (нужны wifi_screen.cpp)
extern lv_obj_t *layer_loading;
extern lv_obj_t *layer_main;
extern lv_obj_t *loading_bar;

// Публичные функции
lv_obj_t *create_card(lv_obj_t *parent, int w, int h);
void ui_init(void);
void loading_timer_cb(lv_timer_t *timer);
