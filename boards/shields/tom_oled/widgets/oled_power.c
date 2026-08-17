/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* OLED の点灯 / 消灯をキーから制御する。
 *
 * ZMK 本体の CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE は、アイドル遷移で必ず
 * display_blanking_off() を呼んでパネルを点け直す。リスナーの実行順は
 * リンク順まかせで制御できないため、本体と同時にブランクを操作すると
 * 「消したはずが打鍵のたびに点く」という競合になる。
 *
 * そこで Kconfig.defconfig で ZMK_DISPLAY_BLANK_ON_IDLE を無効にし、
 * アイドル時のブランクも含めてこのファイルが一元管理する。
 *
 * パネルの状態 = 消灯が選択されていない && アクティビティが ACTIVE
 */

#include "oled_power.h"

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>

#include <zmk/activity.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

#include "oled_mode.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

ZMK_EVENT_IMPL(zmk_tom_oled_power_state);

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

/* パネルに実際に投げた状態。無駄な I2C コマンドを避けるために覚えておく。
 * 起動直後は ZMK 本体が一度 unblank するが、その完了順に依存したくないので
 * UNKNOWN から始めて最初の apply で必ず書き込む。 */
#define PANEL_UNKNOWN (-1)
#define PANEL_OFF 0
#define PANEL_ON 1

static atomic_t panel_state = ATOMIC_INIT(PANEL_UNKNOWN);
static atomic_t power_off = ATOMIC_INIT(0);

bool zmk_tom_oled_power_is_off(void) { return atomic_get(&power_off) != 0; }

static void apply_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (!device_is_ready(display)) {
        return;
    }

    const atomic_val_t want =
        (!zmk_tom_oled_power_is_off() && zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE)
            ? PANEL_ON
            : PANEL_OFF;

    if (atomic_set(&panel_state, want) == want) {
        return;
    }

    if (want == PANEL_ON) {
        display_blanking_off(display);
    } else {
        display_blanking_on(display);
    }
}

K_WORK_DEFINE(apply_work, apply_work_handler);

static void queue_apply(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &apply_work);
    }
}

/* 起動直後の取りこぼし対策。ZMK 本体の表示初期化 (unblank) と設定ロードの
 * どちらが先でも、最後に一度ここで辻褄を合わせる。 */
static void boot_apply_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(boot_apply_work, boot_apply_handler);

static void boot_apply_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (!zmk_display_is_initialized()) {
        k_work_reschedule(&boot_apply_work, K_SECONDS(1));
        return;
    }

    atomic_set(&panel_state, PANEL_UNKNOWN);
    queue_apply();
}

#if IS_ENABLED(CONFIG_SETTINGS)
static void save_power_handler(struct k_work *work) {
    ARG_UNUSED(work);
    uint8_t off = zmk_tom_oled_power_is_off() ? 1 : 0;
    settings_save_one("tom_oled_pwr/off", &off, sizeof(off));
}

K_WORK_DELAYABLE_DEFINE(save_power_work, save_power_handler);

static void queue_power_save(void) {
    k_work_reschedule(&save_power_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
}

/* oled_mode.c が "tom_oled" を使っているので、別サブツリーにする。
 * Zephyr の settings は同名ハンドラを 1 つしか引けない。 */
static int tom_oled_power_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                       void *cb_arg) {
    const char *next;
    uint8_t off;
    int rc;

    if (!settings_name_steq(name, "off", &next) || next != NULL) {
        return -ENOENT;
    }
    if (len != sizeof(off)) {
        return -EINVAL;
    }

    rc = read_cb(cb_arg, &off, sizeof(off));
    if (rc < 0) {
        return rc;
    }

    atomic_set(&power_off, off ? 1 : 0);
    zmk_tom_oled_mode_refresh();
    queue_apply();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(tom_oled_power, "tom_oled_pwr", NULL, tom_oled_power_settings_set,
                               NULL, NULL);
#else
static void queue_power_save(void) {}
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
/* 周辺側が再接続やディープスリープ復帰で状態を取りこぼしても、次に
 * ACTIVE へ上がったときに揃うようにする。打鍵のたびに投げると無駄なので
 * 最短間隔を空ける。 */
#define TOM_OLED_POWER_RESYNC_INTERVAL_MS 60000

/* イベントリスナーや behavior のコールバックから直接 raise しないよう、
 * 一段ワークキューに逃がす。 */
static void broadcast_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    raise_zmk_tom_oled_power_state((struct zmk_tom_oled_power_state){
        .source = ZMK_RELAY_EVENT_SOURCE_SELF,
        .off = zmk_tom_oled_power_is_off() ? 1 : 0,
    });
}

K_WORK_DEFINE(broadcast_work, broadcast_work_handler);

static void broadcast_power_state(void) { k_work_submit(&broadcast_work); }

static void resync_power_state(void) {
    static int64_t last_sent_at;
    static bool have_sent;

    int64_t now = k_uptime_get();
    if (have_sent && now - last_sent_at < TOM_OLED_POWER_RESYNC_INTERVAL_MS) {
        return;
    }

    last_sent_at = now;
    have_sent = true;
    broadcast_power_state();
}
#else
static void broadcast_power_state(void) {}
static void resync_power_state(void) {}
#endif

void zmk_tom_oled_power_toggle(void) {
    atomic_set(&power_off, zmk_tom_oled_power_is_off() ? 0 : 1);
    LOG_INF("tom_oled power off: %d", (int)zmk_tom_oled_power_is_off());

    zmk_tom_oled_mode_refresh();
    queue_apply();
    queue_power_save();
    broadcast_power_state();
}

static int power_activity_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* ディープスリープからの復帰では SSD1306 が Zephyr の PM で resume され、
     * パネルの実状態がこちらの記憶とずれうる。アクティビティが動いたときは
     * キャッシュを捨てて必ず書き直す。 */
    atomic_set(&panel_state, PANEL_UNKNOWN);
    queue_apply();

    if (ev->state == ZMK_ACTIVITY_ACTIVE) {
        resync_power_state();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_tom_oled_power_activity, power_activity_listener);
ZMK_SUBSCRIPTION(widget_tom_oled_power_activity, zmk_activity_state_changed);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT)
static int power_state_listener(const zmk_event_t *eh) {
    const struct zmk_tom_oled_power_state *ev = as_zmk_tom_oled_power_state(eh);
    if (ev == NULL || ev->source == ZMK_RELAY_EVENT_SOURCE_SELF) {
        /* 中央が自分で投げた分は zmk_tom_oled_power_toggle() で処理済み。 */
        return ZMK_EV_EVENT_BUBBLE;
    }

    atomic_set(&power_off, ev->off ? 1 : 0);
    zmk_tom_oled_mode_refresh();
    queue_apply();
    queue_power_save();

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_tom_oled_power_state, power_state_listener);
ZMK_SUBSCRIPTION(widget_tom_oled_power_state, zmk_tom_oled_power_state);

ZMK_RELAY_EVENT_HANDLE(zmk_tom_oled_power_state, top, source);
ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(zmk_tom_oled_power_state, top, source);
#endif

static int tom_oled_power_init(void) {
    k_work_schedule(&boot_apply_work, K_SECONDS(3));
    return 0;
}

SYS_INIT(tom_oled_power_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
