// display_setup.cpp
// ВАЖНО: этот файл НЕ инициализирует LVGL.
// ESPHome (компонент mipi_dsi + lvgl:) сам вызывает lv_init(),
// создаёт lv_display_t, выделяет буферы и регистрирует flush-callback.
// Здесь — только инициализация железа.

#include "display_setup.h"
#include "pins_config.h"
#include "jd9365_lcd.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

extern jd9365_lcd lcd;          // определён в main/.ino или общем файле
extern bool is_asleep;

// ── Инициализация железа (вызывается ДО on_boot ESPHome) ──────────────────
void display_hw_init() {
    // 1. PSRAM — ESPHome тоже его использует, просто проверяем
    if (psramInit()) {
        Serial.printf("[Display] PSRAM OK, free: %d bytes\n",
                      (int)ESP.getFreePsram());
    } else {
        Serial.println("[Display] PSRAM FAILED — дальнейшая работа невозможна");
        return;
    }

    // 2. Подсветка
    pinMode(LCD_LED, OUTPUT);
    digitalWrite(LCD_LED, HIGH);

    // 3. Инициализация LCD-драйвера (MADCTL, питание панели, и т.д.)
    // ПРИМЕЧАНИЕ о повороте:
    //   В YAML стоит rotation: 180 — ESPHome передаст нужный MADCTL
    //   в jd9365-драйвер самостоятельно при инициализации дисплея.
    //   НЕ вызывай здесь ничего, что меняет MADCTL (0x36),
    //   иначе перебьёшь настройку ESPHome.
    lcd.begin();

    Serial.println("[Display] Железо инициализировано, LVGL — за ESPHome");
}

// ── Управление сном ───────────────────────────────────────────────────────
// Подсветкой в ESPHome управляет компонент light: (id: backlight).
// Эти функции оставлены для вызова из UI-кода, но правильнее
// вызывать id(backlight).turn_off() / turn_on() через ESPHome lambda.
// Оставляем как запасной вариант.

bool is_asleep = false;

void go_to_sleep() {
    if (is_asleep) return;
    is_asleep = true;
    digitalWrite(LCD_LED, LOW);
    Serial.println("[Display] Sleep ON");
}

void wake_up() {
    if (!is_asleep) return;
    is_asleep = false;
    digitalWrite(LCD_LED, HIGH);
    lv_display_trigger_activity(NULL);  // сбрасываем таймер бездействия LVGL
    Serial.println("[Display] Wake UP");
}
