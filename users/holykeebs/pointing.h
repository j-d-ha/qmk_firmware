#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "report.h"

typedef enum {
    HK_NONE = 0,
    HK_PIMORONI,
    HK_TRACKPOINT,
    HK_CIRQUE35,
    HK_CIRQUE40,
    HK_AZOTEQ_TPS43,
    HK_AZOTEQ_TPS65,
} hk_pointing_device_type;

typedef enum {
    HK_DEFAULT,
    HK_SNIPING,
    HK_SCROLL_BUFFER,
} hk_cursor_mode;

typedef enum {
    HK_FREE,
    HK_VERTICAL,
    HK_HORIZONTAL,
} hk_scroll_lock;

typedef struct {
    bool                    is_main_side;
    hk_pointing_device_type kind;

    hk_cursor_mode cursor_mode;
    bool           drag_scroll;
    hk_scroll_lock scroll_lock;
    bool           invert_scroll;

    float   default_multiplier;
    float   sniping_multiplier;
    uint8_t scroll_buffer;

    float   rounding_carry_x;
    float   rounding_carry_y;
    int16_t scroll_accum_h;
    int16_t scroll_accum_v;
} hk_pointer_state_t;

typedef struct {
    uint8_t key_row;
    uint8_t key_col;
    bool    key_pressed;
    uint8_t highest_layer;
} hk_display_state_t;

typedef struct {
    bool init;
    bool dirty;
    bool is_main_side;
    bool setting_default_scale;
    bool setting_sniping_scale;
    bool setting_scroll_buffer;

    hk_pointer_state_t main;
    hk_pointer_state_t peripheral;

    hk_display_state_t display;
    report_mouse_t     last_mouse_report;
} hk_state_t;
