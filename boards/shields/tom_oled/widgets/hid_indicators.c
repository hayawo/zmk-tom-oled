/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>

#include "hid_indicators.h"

#define LED_NLCK 0x01
#define LED_CLCK 0x02
#define LED_SLCK 0x04

#define ICON_SIZE 8
#define ICON_GAP 2

LV_IMG_DECLARE(capslock_icon);
LV_IMG_DECLARE(numlock_icon);
LV_IMG_DECLARE(scrolllock_icon);

/* Display order, and the bit that lights each one. */
static const struct {
    const lv_img_dsc_t *dsc;
    uint8_t bit;
} lock_icons[TOM_OLED_LOCK_ICON_COUNT] = {
    {&capslock_icon, LED_CLCK},
    {&numlock_icon, LED_NLCK},
    {&scrolllock_icon, LED_SLCK},
};

struct hid_indicators_state {
    uint8_t hid_indicators;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_hid_indicators(struct zmk_widget_hid_indicators *widget,
                               struct hid_indicators_state state) {
    for (int i = 0; i < TOM_OLED_LOCK_ICON_COUNT; i++) {
        if (state.hid_indicators & lock_icons[i].bit) {
            lv_obj_clear_flag(widget->icons[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->icons[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void hid_indicators_update_cb(struct hid_indicators_state state) {
    struct zmk_widget_hid_indicators *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_hid_indicators(widget, state); }
}

static struct hid_indicators_state hid_indicators_get_state(const zmk_event_t *eh) {
    struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
    return (struct hid_indicators_state) {
        .hid_indicators = ev->indicators,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_hid_indicators, struct hid_indicators_state,
                            hid_indicators_update_cb, hid_indicators_get_state)

ZMK_SUBSCRIPTION(widget_hid_indicators, zmk_hid_indicators_changed);

int zmk_widget_hid_indicators_init(struct zmk_widget_hid_indicators *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj,
                    TOM_OLED_LOCK_ICON_COUNT * (ICON_SIZE + ICON_GAP) - ICON_GAP, ICON_SIZE);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    for (int i = 0; i < TOM_OLED_LOCK_ICON_COUNT; i++) {
        widget->icons[i] = lv_img_create(widget->obj);
        lv_img_set_src(widget->icons[i], lock_icons[i].dsc);
        lv_obj_align(widget->icons[i], LV_ALIGN_TOP_LEFT, i * (ICON_SIZE + ICON_GAP), 0);
        lv_obj_add_flag(widget->icons[i], LV_OBJ_FLAG_HIDDEN);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_hid_indicators_init();

    return 0;
}

lv_obj_t *zmk_widget_hid_indicators_obj(struct zmk_widget_hid_indicators *widget) {
    return widget->obj;
}
