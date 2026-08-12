/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

#include "output_status.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* Generated from PNG files in assets/output_status. Each set bit is a black pixel. */
static const uint16_t icon_usb[] = {0x0fe, 0x082, 0x0aa, 0x082, 0x1ff, 0x101, 0x101,
                                    0x101, 0x101, 0x101, 0x101, 0x101, 0x101, 0x1ff};
static const uint16_t icon_bt[] = {0x07c, 0x0ce, 0x1c7, 0x1d3, 0x119, 0x193, 0x1c7,
                                   0x1c7, 0x193, 0x119, 0x1d3, 0x1c7, 0x0ce, 0x07c};
static const uint16_t icon_ok[] = {0x01, 0x03, 0x16, 0x1c, 0x08};
static const uint16_t icon_nok[] = {0x11, 0x1b, 0x0e, 0x1b, 0x11};
static const uint16_t icon_open[] = {0x04, 0x0e, 0x1b, 0x0e, 0x04};
static const uint16_t icon_profiles[][6] = {
    {0x06, 0x0e, 0x0e, 0x06, 0x06, 0x06},
    {0x0e, 0x1b, 0x03, 0x06, 0x0c, 0x1f},
    {0x0e, 0x13, 0x06, 0x03, 0x1b, 0x0e},
    {0x02, 0x06, 0x0e, 0x1a, 0x1f, 0x02},
    {0x0f, 0x08, 0x0e, 0x03, 0x1b, 0x0e},
};

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    enum zmk_transport preferred_transport;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

static void draw_bitmap(lv_layer_t *layer, const lv_area_t *coords, int32_t offset_x,
                        int32_t offset_y, const uint16_t *rows, int32_t width, int32_t height) {
    lv_draw_rect_dsc_t pixel_dsc;
    lv_draw_rect_dsc_init(&pixel_dsc);
    pixel_dsc.bg_color = lv_color_black();
    pixel_dsc.bg_opa = LV_OPA_COVER;
    pixel_dsc.border_opa = LV_OPA_TRANSP;
    pixel_dsc.radius = 0;

    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            if ((rows[y] & BIT(width - 1 - x)) != 0) {
                lv_area_t pixel = {
                    .x1 = coords->x1 + offset_x + x,
                    .y1 = coords->y1 + offset_y + y,
                    .x2 = coords->x1 + offset_x + x,
                    .y2 = coords->y1 + offset_y + y,
                };
                lv_draw_rect(layer, &pixel_dsc, &pixel);
            }
        }
    }
}

static void draw_output_status(lv_event_t *event) {
    struct zmk_widget_output_status *widget = lv_event_get_user_data(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t coords;
    lv_obj_get_coords(widget->obj, &coords);

    /* This widget draws directly onto the display layer. Clear its previous
     * frame first so endpoint bars and status marks do not accumulate when an
     * endpoint event invalidates the transparent object. */
    lv_draw_rect_dsc_t clear_dsc;
    lv_draw_rect_dsc_init(&clear_dsc);
    clear_dsc.bg_color = lv_color_white();
    clear_dsc.bg_opa = LV_OPA_COVER;
    clear_dsc.border_opa = LV_OPA_TRANSP;
    clear_dsc.radius = 0;
    lv_draw_rect(layer, &clear_dsc, &coords);

    draw_bitmap(layer, &coords, 1, 4, icon_usb, 9, 14);
    draw_bitmap(layer, &coords, 3, 6, widget->usb_is_hid_ready ? icon_ok : icon_nok, 5, 5);
    draw_bitmap(layer, &coords, 16, 4, icon_bt, 9, 14);
    draw_bitmap(layer, &coords, 27, 11, icon_profiles[widget->active_profile_index], 5, 6);
    draw_bitmap(layer, &coords, 27, 5,
                !widget->active_profile_bonded
                    ? icon_open
                    : (widget->active_profile_connected ? icon_ok : icon_nok),
                5, 5);

    lv_draw_rect_dsc_t line_dsc;
    lv_draw_rect_dsc_init(&line_dsc);
    line_dsc.bg_color = lv_color_black();
    line_dsc.bg_opa = LV_OPA_COVER;
    line_dsc.border_opa = LV_OPA_TRANSP;
    line_dsc.radius = 0;
    lv_area_t line = {
        .x1 = coords.x1 + (widget->ble_selected ? 15 : 0),
        .y1 = coords.y1 + 1,
        .x2 = coords.x1 + (widget->ble_selected ? 32 : 10),
        .y2 = coords.y1 + 2,
    };
    lv_draw_rect(layer, &line_dsc, &line);
}

static struct output_status_state get_state(const zmk_event_t *_eh) {
    ARG_UNUSED(_eh);
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoint_get_selected(),
        .preferred_transport = zmk_endpoint_get_preferred_transport(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
        .usb_is_hid_ready = zmk_usb_is_hid_ready(),
    };
}

static void set_status(struct zmk_widget_output_status *widget,
                       struct output_status_state state) {
    enum zmk_transport transport = state.selected_endpoint.transport;
    if (transport == ZMK_TRANSPORT_NONE) {
        transport = state.preferred_transport;
    }

    int profile = state.active_profile_index;
    if (profile < 0 || profile >= ARRAY_SIZE(icon_profiles)) {
        profile = 0;
    }
    widget->active_profile_index = profile;
    widget->ble_selected = transport == ZMK_TRANSPORT_BLE;
    widget->active_profile_connected = state.active_profile_connected;
    widget->active_profile_bonded = state.active_profile_bonded;
    widget->usb_is_hid_ready = state.usb_is_hid_ready;
    lv_obj_invalidate(widget->obj);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 38, 20);
    widget->active_profile_index = 0;
    widget->active_profile_connected = false;
    widget->active_profile_bonded = false;
    widget->usb_is_hid_ready = false;
    widget->ble_selected = false;
    lv_obj_add_event_cb(widget->obj, draw_output_status, LV_EVENT_DRAW_MAIN, widget);

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}
