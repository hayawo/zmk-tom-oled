/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "codex_status.h"

#include <pb_encode.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/byteorder.h>
#include <zmk/studio/custom.h>

#define CODEX_STATUS_PROTOCOL_VERSION 1
#define CODEX_STATUS_REQUEST_SIZE 12
#define CODEX_STATUS_RESPONSE_SIZE 4

enum response_code {
    RESPONSE_OK = 0,
    RESPONSE_INVALID_LENGTH = 1,
    RESPONSE_INVALID_VERSION = 2,
    RESPONSE_INVALID_STATUS = 3,
    RESPONSE_STALE_SEQUENCE = 4,
};

static struct zmk_rpc_custom_subsystem_meta codex_status_meta = {
    .ui_urls = NULL,
    .ui_urls_count = 0,
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

static uint8_t response_payload[CODEX_STATUS_RESPONSE_SIZE];
static bool have_sequence;
static uint32_t current_session;
static uint32_t current_sequence;

static bool encode_response_payload(pb_ostream_t *stream, const pb_field_t *field,
                                    void *const *arg) {
    const uint8_t *payload = *arg;
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(stream, payload, CODEX_STATUS_RESPONSE_SIZE);
}

static void prepare_response(pb_callback_t *encode_response, enum response_code code,
                             enum zmk_tom_oled_codex_status status) {
    response_payload[0] = CODEX_STATUS_PROTOCOL_VERSION;
    response_payload[1] = code;
    response_payload[2] = status;
    response_payload[3] = 0;
    encode_response->funcs.encode = encode_response_payload;
    encode_response->arg = response_payload;
}

static bool sequence_is_newer(uint32_t session, uint32_t sequence) {
    if (!have_sequence || session != current_session) {
        return true;
    }
    return (int32_t)(sequence - current_sequence) > 0;
}

ZMK_RPC_CUSTOM_SUBSYSTEM(tom_oled__codex_status, &codex_status_meta,
                         codex_status_rpc_handle_request);

static bool codex_status_rpc_handle_request(const zmk_custom_CallRequest *request,
                                            pb_callback_t *encode_response) {
    enum zmk_tom_oled_codex_status existing = zmk_tom_oled_codex_status_get();
    prepare_response(encode_response, RESPONSE_OK, existing);

    if (request->payload.size != CODEX_STATUS_REQUEST_SIZE) {
        prepare_response(encode_response, RESPONSE_INVALID_LENGTH, existing);
        return true;
    }

    const uint8_t *payload = request->payload.bytes;
    if (payload[0] != CODEX_STATUS_PROTOCOL_VERSION) {
        prepare_response(encode_response, RESPONSE_INVALID_VERSION, existing);
        return true;
    }

    enum zmk_tom_oled_codex_status status = payload[1];
    if (status > ZMK_TOM_OLED_CODEX_STATUS_FAIL) {
        prepare_response(encode_response, RESPONSE_INVALID_STATUS, existing);
        return true;
    }

    uint16_t ttl_seconds = sys_get_le16(&payload[2]);
    uint32_t session = sys_get_le32(&payload[4]);
    uint32_t sequence = sys_get_le32(&payload[8]);
    if (!sequence_is_newer(session, sequence)) {
        prepare_response(encode_response, RESPONSE_STALE_SEQUENCE, existing);
        return true;
    }

    if (status != ZMK_TOM_OLED_CODEX_STATUS_OFF) {
        if (ttl_seconds == 0) {
            ttl_seconds = CONFIG_ZMK_TOM_OLED_CODEX_STATUS_DEFAULT_TTL_SEC;
        } else if (ttl_seconds > CONFIG_ZMK_TOM_OLED_CODEX_STATUS_MAX_TTL_SEC) {
            ttl_seconds = CONFIG_ZMK_TOM_OLED_CODEX_STATUS_MAX_TTL_SEC;
        }
    }

    current_session = session;
    current_sequence = sequence;
    have_sequence = true;
    zmk_tom_oled_codex_status_set(status, ttl_seconds);
    prepare_response(encode_response, RESPONSE_OK, status);
    return true;
}
