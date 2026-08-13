/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_peripheral_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *canvas;
    lv_obj_t *connection_label;
    lv_obj_t *mode_label;
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    lv_obj_t *battery_label;
#endif
    lv_timer_t *anim_timer;
    lv_color_t cbuf[64 * 32];
    bool connected;
    bool moving;
    bool typing;
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    uint8_t battery_level;
#endif
    uint8_t frame;
    int64_t moving_until;
    int64_t typing_until;
};

int zmk_widget_peripheral_status_init(struct zmk_widget_peripheral_status *widget,
                                      lv_obj_t *parent);
lv_obj_t *zmk_widget_peripheral_status_obj(struct zmk_widget_peripheral_status *widget);
