/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_tom_oled_layer_activity {
    uint8_t source;
    uint8_t layer;
};

ZMK_EVENT_DECLARE(zmk_tom_oled_layer_activity);
