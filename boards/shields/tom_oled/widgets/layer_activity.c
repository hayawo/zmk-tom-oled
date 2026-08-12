/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "layer_activity.h"

#include <zmk/event_manager.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/split/central.h>
#endif

ZMK_EVENT_IMPL(zmk_tom_oled_layer_activity);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static int layer_state_listener(const zmk_event_t *eh) {
    if (as_zmk_layer_state_changed(eh) == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    raise_zmk_tom_oled_layer_activity((struct zmk_tom_oled_layer_activity){
        .source = ZMK_RELAY_EVENT_SOURCE_SELF,
        .layer = zmk_keymap_highest_layer_active(),
    });
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(tom_oled_layer_state, layer_state_listener);
ZMK_SUBSCRIPTION(tom_oled_layer_state, zmk_layer_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
ZMK_RELAY_EVENT_HANDLE(zmk_tom_oled_layer_activity, tla, source);
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(zmk_tom_oled_layer_activity, tla, source);
#endif

#endif
