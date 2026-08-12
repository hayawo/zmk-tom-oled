/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"

#include <zephyr/sys/util.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/display/widgets/battery_status.h>
#include "widgets/modifiers.h"
#include "widgets/codex_status.h"
#include "widgets/bongo_cat.h"
#include "widgets/oled_mode.h"
#include "widgets/layer_status.h"
#include "widgets/output_status.h"
#include "widgets/hid_indicators.h"
#else
#include "widgets/peripheral_status.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_modifiers modifiers_widget;
static struct zmk_widget_codex_status codex_status_widget;
static struct zmk_widget_bongo_cat bongo_cat_widget;

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
static struct zmk_widget_hid_indicators hid_indicators_widget;
#endif
#else
static struct zmk_widget_peripheral_status peripheral_status_widget;
#endif

lv_style_t global_style;

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 1
static void delayed_output_status_init(lv_timer_t *timer) {
    lv_obj_t *screen = lv_timer_get_user_data(timer);

    LOG_INF("Starting delayed output status widget");
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    LOG_INF("Delayed output status widget initialized");
    lv_timer_delete(timer);
}
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;

    screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_bg_color(&global_style, lv_color_white());
    lv_style_set_bg_opa(&global_style, LV_OPA_COVER);
    lv_style_set_text_color(&global_style, lv_color_black());
    lv_style_set_text_font(&global_style, &lv_font_unscii_8);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 1 || \
    CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 5 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 6 || \
    CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 7 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 8
#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 1
    lv_obj_t *diagnostic_label = lv_label_create(screen);
    lv_label_set_text(diagnostic_label, "OLED DEBUG\nWAIT 10 SEC");
    lv_obj_center(diagnostic_label);
    lv_timer_create(delayed_output_status_init, 10000, screen);
#else
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif
#endif

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 3
    zmk_widget_codex_status_init(&codex_status_widget, screen);
    lv_obj_align(zmk_widget_codex_status_obj(&codex_status_widget), LV_ALIGN_CENTER, 0, -7);
#endif

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 2 || \
    CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 8
    zmk_widget_bongo_cat_init(&bongo_cat_widget, screen);
    lv_obj_align(zmk_widget_bongo_cat_obj(&bongo_cat_widget), LV_ALIGN_CENTER, 0, -7);
#endif

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0
    zmk_tom_oled_mode_display_init(zmk_widget_bongo_cat_obj(&bongo_cat_widget),
                                   zmk_widget_codex_status_obj(&codex_status_widget));
#endif

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 4
    zmk_widget_modifiers_init(&modifiers_widget, screen);
    lv_obj_align(zmk_widget_modifiers_obj(&modifiers_widget), LV_ALIGN_BOTTOM_LEFT, 0, 2);

// #if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
//     zmk_widget_hid_indicators_init(&hid_indicators_widget, screen);
//     lv_obj_align_to(zmk_widget_hid_indicators_obj(&hid_indicators_widget), zmk_widget_modifiers_obj(&modifiers_widget), LV_ALIGN_OUT_TOP_LEFT, 0, -2);
// #endif

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    // lv_obj_align_to(zmk_widget_layer_status_obj(&layer_status_widget), zmk_widget_bongo_cat_obj(&bongo_cat_widget), LV_ALIGN_BOTTOM_LEFT, 0, 5);
#endif

#if CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 0 || CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 5 || \
    CONFIG_ZMK_TOM_OLED_DIAGNOSTIC_STAGE == 8
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_t *battery_obj = zmk_widget_battery_status_obj(&battery_status_widget);
    /* The ZMK widget prepends LV_SYMBOL_CHARGE while USB is powered. UNASCII 8
     * does not contain that symbol and renders it as a large missing-glyph box. */
    lv_obj_set_style_text_font(battery_obj, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(battery_obj, 0, LV_PART_MAIN);
    lv_obj_align(battery_obj, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif
#else
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}
