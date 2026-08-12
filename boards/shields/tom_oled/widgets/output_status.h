/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
 
 #pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_output_status {
    sys_snode_t node;
    lv_obj_t *obj;
    uint8_t active_profile_index;
    bool ble_selected;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget);
