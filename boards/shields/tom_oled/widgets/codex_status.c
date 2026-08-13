/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "codex_status.h"

#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zmk/display.h>

#include "assets/codex_status_images.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static atomic_t current_status = ATOMIC_INIT(ZMK_TOM_OLED_CODEX_STATUS_OFF);

ZMK_EVENT_IMPL(zmk_tom_oled_codex_status_changed);

static void publish_status(enum zmk_tom_oled_codex_status status) {
    atomic_set(&current_status, status);
    raise_zmk_tom_oled_codex_status_changed(
        (struct zmk_tom_oled_codex_status_changed){.status = status});
}

static void status_timeout_cb(struct k_work *work) {
    ARG_UNUSED(work);
    publish_status(ZMK_TOM_OLED_CODEX_STATUS_OFF);
}

static K_WORK_DELAYABLE_DEFINE(status_timeout_work, status_timeout_cb);

int zmk_tom_oled_codex_status_set(enum zmk_tom_oled_codex_status status,
                                  uint16_t ttl_seconds) {
    if (status < ZMK_TOM_OLED_CODEX_STATUS_OFF || status > ZMK_TOM_OLED_CODEX_STATUS_FAIL) {
        return -EINVAL;
    }

    if (status == ZMK_TOM_OLED_CODEX_STATUS_OFF || ttl_seconds == 0) {
        k_work_cancel_delayable(&status_timeout_work);
    } else {
        k_work_reschedule(&status_timeout_work, K_SECONDS(ttl_seconds));
    }

    publish_status(status);
    return 0;
}

enum zmk_tom_oled_codex_status zmk_tom_oled_codex_status_get(void) {
    return (enum zmk_tom_oled_codex_status)atomic_get(&current_status);
}

static const lv_image_dsc_t *image_for_status(enum zmk_tom_oled_codex_status status) {
    switch (status) {
    case ZMK_TOM_OLED_CODEX_STATUS_WORK:
        return &codex_status_work;
    case ZMK_TOM_OLED_CODEX_STATUS_WAIT:
        return &codex_status_wait;
    case ZMK_TOM_OLED_CODEX_STATUS_DONE:
        return &codex_status_done;
    case ZMK_TOM_OLED_CODEX_STATUS_FAIL:
        return &codex_status_fail;
    case ZMK_TOM_OLED_CODEX_STATUS_OFF:
    default:
        return &codex_status_off;
    }
}

static void set_status(lv_obj_t *image, enum zmk_tom_oled_codex_status status) {
    lv_image_set_src(image, image_for_status(status));
}

static void codex_status_update_cb(enum zmk_tom_oled_codex_status status) {
    struct zmk_widget_codex_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_status(widget->obj, status); }
}

static enum zmk_tom_oled_codex_status codex_status_get_state(const zmk_event_t *eh) {
    const struct zmk_tom_oled_codex_status_changed *event =
        eh != NULL ? as_zmk_tom_oled_codex_status_changed(eh) : NULL;
    return event == NULL ? zmk_tom_oled_codex_status_get() : event->status;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_codex_status, enum zmk_tom_oled_codex_status,
                            codex_status_update_cb, codex_status_get_state)
ZMK_SUBSCRIPTION(widget_codex_status, zmk_tom_oled_codex_status_changed);

int zmk_widget_codex_status_init(struct zmk_widget_codex_status *widget, lv_obj_t *parent) {
    widget->obj = lv_image_create(parent);
    lv_image_set_src(widget->obj, image_for_status(zmk_tom_oled_codex_status_get()));

    sys_slist_append(&widgets, &widget->node);
    widget_codex_status_init();
    return 0;
}

lv_obj_t *zmk_widget_codex_status_obj(struct zmk_widget_codex_status *widget) {
    return widget->obj;
}
