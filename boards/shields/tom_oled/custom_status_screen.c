/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"

#include <zephyr/sys/util.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include "widgets/battery_status.h"
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
static struct zmk_widget_dongle_battery_status dongle_battery_status_widget;
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

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;

    screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_text_font(&global_style, &lv_font_unscii_8);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    
    zmk_widget_codex_status_init(&codex_status_widget, screen);
    lv_obj_align(zmk_widget_codex_status_obj(&codex_status_widget), LV_ALIGN_CENTER, 0, -7);

    zmk_widget_bongo_cat_init(&bongo_cat_widget, screen);
    lv_obj_align(zmk_widget_bongo_cat_obj(&bongo_cat_widget), LV_ALIGN_CENTER, 0, -7);
    zmk_tom_oled_mode_display_init(zmk_widget_bongo_cat_obj(&bongo_cat_widget),
                                   zmk_widget_codex_status_obj(&codex_status_widget));

    zmk_widget_modifiers_init(&modifiers_widget, screen);
    lv_obj_align(zmk_widget_modifiers_obj(&modifiers_widget), LV_ALIGN_BOTTOM_LEFT, 0, 2);

    // 修飾キーの右、レイヤー表示の左の最下段に置く。
    //   修飾キー   : 4 シンボル * (14+1) + 1 = 61px 幅 -> x 0..61
    //   レイヤー   : 幅 18px の右下寄せ         -> x 110..128
    //   Bongo Cat  : 50x26 を y=-7 で中央寄せ    -> 下端は y 22
    // よって x 64..110 / y 24..32 が空く。頭文字 1 文字ずつなので 3 つ点灯
    // しても 27px で収まる。
    // 当初は出力状態の下 (x 0..38 / y 7..15) に置いたが、USB/Bluetooth の
    // 表示に重なったため移動した。
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    zmk_widget_hid_indicators_init(&hid_indicators_widget, screen);
    lv_obj_align(zmk_widget_hid_indicators_obj(&hid_indicators_widget), LV_ALIGN_BOTTOM_LEFT, 64, 0);
#endif

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    // lv_obj_align_to(zmk_widget_layer_status_obj(&layer_status_widget), zmk_widget_bongo_cat_obj(&bongo_cat_widget), LV_ALIGN_BOTTOM_LEFT, 0, 5);

    zmk_widget_dongle_battery_status_init(&dongle_battery_status_widget, screen);
    lv_obj_align(zmk_widget_dongle_battery_status_obj(&dongle_battery_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);
#else
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}
