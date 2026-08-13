/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Lock indicator icons, 8x8 so three of them fit the free strip between the
 * modifier row and the layer counter (x 64..110, y 24..32) on a 128x32 panel.
 * Same 1-bit indexed format as modifiers_sym.c: an 8 byte palette followed by
 * one byte per row.
 */

#include <lvgl.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_CONTROL
#define LV_ATTRIBUTE_IMG_CONTROL
#endif

/* Caps lock: arrow over a bar (the usual U+21EA shape).
 *
 *      ...##...
 *      ..####..
 *      .######.
 *      ########
 *      ..####..
 *      ..####..
 *      ........
 *      ########
 */
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST
    LV_ATTRIBUTE_IMG_CONTROL uint8_t capslock_map[] = {
        0xff, 0xff, 0xff, 0xff, /*Color of index 0*/
        0x00, 0x00, 0x00, 0xff, /*Color of index 1*/

        0x18, 0x3c, 0x7e, 0xff, 0x3c, 0x3c, 0x00, 0xff,
};

const lv_img_dsc_t capslock_icon = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 8,
    .header.h = 8,
    .data_size = 16,
    .data = capslock_map,
};

/* Num lock: a boxed "1".
 *
 *      ########
 *      #......#
 *      #..##..#
 *      #.###..#
 *      #..##..#
 *      #.####.#
 *      #......#
 *      ########
 */
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST
    LV_ATTRIBUTE_IMG_CONTROL uint8_t numlock_map[] = {
        0xff, 0xff, 0xff, 0xff, /*Color of index 0*/
        0x00, 0x00, 0x00, 0xff, /*Color of index 1*/

        0xff, 0x81, 0x99, 0xb9, 0x99, 0xbd, 0x81, 0xff,
};

const lv_img_dsc_t numlock_icon = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 8,
    .header.h = 8,
    .data_size = 16,
    .data = numlock_map,
};

/* Scroll lock: arrow pointing down over a bar, mirroring the caps icon.
 *
 *      ..####..
 *      ..####..
 *      ########
 *      .######.
 *      ..####..
 *      ...##...
 *      ........
 *      ########
 */
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST
    LV_ATTRIBUTE_IMG_CONTROL uint8_t scrolllock_map[] = {
        0xff, 0xff, 0xff, 0xff, /*Color of index 0*/
        0x00, 0x00, 0x00, 0xff, /*Color of index 1*/

        0x3c, 0x3c, 0xff, 0x7e, 0x3c, 0x18, 0x00, 0xff,
};

const lv_img_dsc_t scrolllock_icon = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 8,
    .header.h = 8,
    .data_size = 16,
    .data = scrolllock_map,
};
