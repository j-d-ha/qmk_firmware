#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pointing.h"

#define HK_EEPROM_VERSION 100
#define HK_EEPROM_CHECK 0xD4

typedef struct {
    hk_cursor_mode cursor_mode;
    bool           drag_scroll;
    hk_scroll_lock scroll_lock;
    bool           invert_scroll;

    uint16_t default_multiplier_x100;
    uint16_t sniping_multiplier_x100;
    uint8_t  scroll_buffer;
} hk_pointer_eeprom_t;

typedef struct {
    uint8_t version;
    uint8_t check;

    hk_pointer_eeprom_t main;
    hk_pointer_eeprom_t peripheral;
} hk_eeprom_config_t;
