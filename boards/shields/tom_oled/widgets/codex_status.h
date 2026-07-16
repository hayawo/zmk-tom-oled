/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

enum zmk_tom_oled_codex_status {
    ZMK_TOM_OLED_CODEX_STATUS_OFF = 0,
    ZMK_TOM_OLED_CODEX_STATUS_WORK = 1,
    ZMK_TOM_OLED_CODEX_STATUS_WAIT = 2,
    ZMK_TOM_OLED_CODEX_STATUS_DONE = 3,
    ZMK_TOM_OLED_CODEX_STATUS_FAIL = 4,
};

struct zmk_tom_oled_codex_status_changed {
    enum zmk_tom_oled_codex_status status;
};

ZMK_EVENT_DECLARE(zmk_tom_oled_codex_status_changed);

struct zmk_widget_codex_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_tom_oled_codex_status_set(enum zmk_tom_oled_codex_status status,
                                  uint16_t ttl_seconds);
enum zmk_tom_oled_codex_status zmk_tom_oled_codex_status_get(void);

int zmk_widget_codex_status_init(struct zmk_widget_codex_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_codex_status_obj(struct zmk_widget_codex_status *widget);
