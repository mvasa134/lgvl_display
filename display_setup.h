#pragma once

// display_setup.h
// В режиме ESPHome-хоста этот файл содержит только
// инициализацию железа (PSRAM, подсветка, lcd.begin).
// lv_init(), lv_display_create() и flush-callback
// полностью берёт на себя ESPHome mipi_dsi компонент.

void display_hw_init();   // переименовано: НЕ трогает LVGL
void go_to_sleep();
void wake_up();
