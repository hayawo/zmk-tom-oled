/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "trackball_activity.h"

#include <stdbool.h>
#include <string.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

#define TOM_OLED_TRACKBALL_RELAY_INTERVAL_MS 500

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

ZMK_EVENT_IMPL(zmk_tom_oled_trackball_activity);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)

static void raise_trackball_activity(void) {
    struct zmk_tom_oled_trackball_activity event = {0};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
    event.source = ZMK_RELAY_EVENT_SOURCE_SELF;
#endif

    raise_zmk_tom_oled_trackball_activity(event);
}

static void trackball_input_listener(struct input_event *ev, void *user_data) {
    ARG_UNUSED(user_data);

    if (ev->type != INPUT_EV_REL || ev->value == 0) {
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    static int64_t last_relay_at;
    static bool have_relayed;
    int64_t now = k_uptime_get();

    if (!have_relayed || now - last_relay_at >= TOM_OLED_TRACKBALL_RELAY_INTERVAL_MS) {
        last_relay_at = now;
        have_relayed = true;
        raise_trackball_activity();
    }
#else
    raise_trackball_activity();
#endif
}

INPUT_CALLBACK_DEFINE(NULL, trackball_input_listener, NULL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
ZMK_RELAY_EVENT_HANDLE(zmk_tom_oled_trackball_activity, tpa, source);
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(zmk_tom_oled_trackball_activity, tpa, source);
#endif

#endif
