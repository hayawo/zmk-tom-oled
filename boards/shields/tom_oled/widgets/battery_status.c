/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/split/central.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>

#include "battery_status.h"

#if IS_ENABLED(CONFIG_ZMK_TOM_OLED_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
    bool present;
};

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
    lv_obj_t *fill;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) {
        return;
    }
    LOG_DBG("source: %d, level: %d, usb: %d", state.source, state.level, state.usb_present);
    struct battery_object *battery = &battery_objects[state.source];
    lv_obj_t *symbol = battery->symbol;
    lv_obj_t *label = battery->label;

    int32_t fill_height = state.usb_present || state.level <= 10 ? 5
                          : state.level <= 30                      ? 4
                          : state.level <= 50                      ? 3
                          : state.level <= 70                      ? 2
                          : state.level <= 90                      ? 1
                                                                  : 0;
    if (fill_height == 0) {
        lv_obj_add_flag(battery->fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(battery->fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(battery->fill, 3, fill_height);
        lv_obj_set_pos(battery->fill, 1, 2);
        lv_obj_set_style_bg_opa(battery->fill,
                                state.usb_present ? LV_OPA_TRANSP : LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(battery->fill, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_border_opa(battery->fill,
                                    state.usb_present ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(battery->fill, state.usb_present ? 1 : 0, LV_PART_MAIN);
    }
    lv_label_set_text_fmt(label, "%4u%%", state.level);
    
    if (state.present || state.usb_present) {
        lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
}

void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){
        .source = ev->source + SOURCE_OFFSET,
        .level = ev->state_of_charge,
        .present = true,
    };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev =
        eh != NULL ? as_zmk_battery_state_changed(eh) : NULL;
    return (struct battery_state) {
        .source = 0,
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
        .present = true,
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) { 
    if (eh != NULL && as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    } else {
        return central_battery_status_get_state(eh);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_TOM_OLED_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
#endif /* !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */
#endif /* IS_ENABLED(CONFIG_ZMK_TOM_OLED_DONGLE_BATTERY) */

int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 42,
                    (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) * 10);
    
    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *battery_symbol = lv_obj_create(widget->obj);
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_obj_remove_style_all(battery_symbol);
        lv_obj_set_size(battery_symbol, 5, 8);

        lv_obj_t *cap_left = lv_obj_create(battery_symbol);
        lv_obj_remove_style_all(cap_left);
        lv_obj_set_size(cap_left, 1, 1);
        lv_obj_set_pos(cap_left, 0, 0);
        lv_obj_set_style_bg_color(cap_left, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cap_left, LV_OPA_COVER, LV_PART_MAIN);

        lv_obj_t *cap_right = lv_obj_create(battery_symbol);
        lv_obj_remove_style_all(cap_right);
        lv_obj_set_size(cap_right, 1, 1);
        lv_obj_set_pos(cap_right, 4, 0);
        lv_obj_set_style_bg_color(cap_right, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cap_right, LV_OPA_COVER, LV_PART_MAIN);

        lv_obj_t *battery_fill = lv_obj_create(battery_symbol);
        lv_obj_remove_style_all(battery_fill);
        lv_obj_set_style_bg_color(battery_fill, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(battery_fill, 0, LV_PART_MAIN);

        battery_objects[i] = (struct battery_object){
            .symbol = battery_symbol,
            .label = battery_label,
            .fill = battery_fill,
        };

        lv_obj_align(battery_symbol, LV_ALIGN_TOP_RIGHT, 0, i * 10);
        lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -7, i * 10);

        lv_obj_add_flag(battery_symbol, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_dongle_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
