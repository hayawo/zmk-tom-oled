/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_tom_oled_trackball_activity {
    uint8_t source;
};

ZMK_EVENT_DECLARE(zmk_tom_oled_trackball_activity);
