/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "oled_mode.h"

#include <errno.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zmk/display.h>

#include "oled_power.h"

static void queue_mode_apply(void);

static atomic_t current_mode = ATOMIC_INIT(IS_ENABLED(CONFIG_ZMK_TOM_OLED_CODEX_STATUS)
                                               ? ZMK_TOM_OLED_MODE_AGENT
                                               : ZMK_TOM_OLED_MODE_BONGO);
static lv_obj_t *bongo_widget;
static lv_obj_t *agent_widget;

static void set_hidden(lv_obj_t *obj, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void apply_mode(struct k_work *work) {
    ARG_UNUSED(work);

    if (bongo_widget == NULL || agent_widget == NULL) {
        return;
    }

    /* 消灯中は両方隠す。Bongo Cat は WPM が 0 でも lv_animimg を
     * LV_ANIM_REPEAT_INFINITE で回し続けるため、隠さないとパネルを
     * ブランクしても LVGL の再描画と I2C 転送が走り続ける。 */
    const bool off = zmk_tom_oled_power_is_off();
    const bool agent = zmk_tom_oled_mode_get() == ZMK_TOM_OLED_MODE_AGENT;

    set_hidden(bongo_widget, off || agent);
    set_hidden(agent_widget, off || !agent);
}

K_WORK_DEFINE(apply_mode_work, apply_mode);

static void queue_mode_apply(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &apply_mode_work);
    }
}

void zmk_tom_oled_mode_refresh(void) { queue_mode_apply(); }

enum zmk_tom_oled_mode zmk_tom_oled_mode_get(void) {
    return (enum zmk_tom_oled_mode)atomic_get(&current_mode);
}

#if IS_ENABLED(CONFIG_SETTINGS)
static void save_mode(struct k_work *work) {
    ARG_UNUSED(work);
    uint8_t mode = (uint8_t)zmk_tom_oled_mode_get();
    settings_save_one("tom_oled/mode", &mode, sizeof(mode));
}

K_WORK_DELAYABLE_DEFINE(save_mode_work, save_mode);

static void queue_mode_save(void) {
    k_work_reschedule(&save_mode_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
}

static int tom_oled_mode_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                      void *cb_arg) {
    const char *next;
    uint8_t mode;
    int rc;

    if (!settings_name_steq(name, "mode", &next) || next != NULL) {
        return -ENOENT;
    }
    if (len != sizeof(mode)) {
        return -EINVAL;
    }

    rc = read_cb(cb_arg, &mode, sizeof(mode));
    if (rc < 0) {
        return rc;
    }
    if (mode > ZMK_TOM_OLED_MODE_AGENT) {
        return -EINVAL;
    }

    atomic_set(&current_mode, mode);
    queue_mode_apply();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(tom_oled_mode, "tom_oled", NULL, tom_oled_mode_settings_set, NULL,
                               NULL);
#else
static void queue_mode_save(void) {}
#endif

void zmk_tom_oled_mode_toggle(void) {
    enum zmk_tom_oled_mode next = zmk_tom_oled_mode_get() == ZMK_TOM_OLED_MODE_AGENT
                                      ? ZMK_TOM_OLED_MODE_BONGO
                                      : ZMK_TOM_OLED_MODE_AGENT;
    atomic_set(&current_mode, next);
    queue_mode_apply();
    queue_mode_save();
}

void zmk_tom_oled_mode_display_init(lv_obj_t *bongo, lv_obj_t *agent) {
    bongo_widget = bongo;
    agent_widget = agent;
    apply_mode(NULL);
}
