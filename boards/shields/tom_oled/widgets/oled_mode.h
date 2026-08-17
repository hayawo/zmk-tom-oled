/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

enum zmk_tom_oled_mode {
    ZMK_TOM_OLED_MODE_BONGO = 0,
    ZMK_TOM_OLED_MODE_AGENT = 1,
};

enum zmk_tom_oled_mode zmk_tom_oled_mode_get(void);
void zmk_tom_oled_mode_toggle(void);
/* 表示内容を現在のモード / 消灯状態に合わせ直す。周辺側では何もしない。 */
void zmk_tom_oled_mode_refresh(void);
void zmk_tom_oled_mode_display_init(lv_obj_t *bongo_widget, lv_obj_t *agent_widget);
