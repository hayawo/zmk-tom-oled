/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>

/* 消灯状態を中央から周辺へ伝えるリレーイベント。
 * source は ZMK のリレー機構が使う。中央が自分で投げたものは
 * ZMK_RELAY_EVENT_SOURCE_SELF、周辺が受け取り直したものはそれ以外になる。 */
struct zmk_tom_oled_power_state {
    uint8_t source;
    uint8_t off;
};

ZMK_EVENT_DECLARE(zmk_tom_oled_power_state);

#if IS_ENABLED(CONFIG_ZMK_TOM_OLED_POWER_CONTROL)

/* 消灯が選択されているか。左右どちらの側でも同じ答えを返す。 */
bool zmk_tom_oled_power_is_off(void);

/* 点灯 / 消灯を切り替える。中央側から呼ぶこと (behavior は CENTRAL locality)。
 * 設定は NVS に保存され、周辺側へリレーされる。 */
void zmk_tom_oled_power_toggle(void);

#else

/* 機能を切ってビルドしたときも呼び出し側を #if で汚さずに済むようにする。 */
static inline bool zmk_tom_oled_power_is_off(void) { return false; }

#endif
