#include "holykeebs.h"

#include <math.h>

#include "action_layer.h"
#include "eeconfig.h"
#include "eeprom_config.h"
#include "hk_debug.h"

hk_state_t         g_hk_state        = {0};
static bool        g_settings_loaded = false;

static hk_pointer_state_t *hk_pointer(bool peripheral) {
    return peripheral ? &g_hk_state.peripheral : &g_hk_state.main;
}

static hk_pointing_device_type hk_detect_kind_main(void) {
#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) || defined(HK_POINTING_DEVICE_RIGHT_PIMORONI)
    return HK_PIMORONI;
#elif defined(HK_POINTING_DEVICE_LEFT_TRACKPOINT) || defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    return HK_TRACKPOINT;
#else
    return HK_NONE;
#endif
}

static hk_pointing_device_type hk_detect_kind_peripheral(void) {
#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) && defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    return HK_PIMORONI;
#elif defined(HK_POINTING_DEVICE_LEFT_TRACKPOINT) && defined(HK_POINTING_DEVICE_RIGHT_PIMORONI)
    return HK_PIMORONI;
#elif defined(HK_POINTING_DEVICE_LEFT_PIMORONI) || defined(HK_POINTING_DEVICE_RIGHT_PIMORONI)
    return HK_NONE;
#elif defined(HK_POINTING_DEVICE_LEFT_TRACKPOINT) || defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    return HK_NONE;
#else
    return HK_NONE;
#endif
}

static void hk_set_pointer_defaults(hk_pointer_state_t *pointer) {
    pointer->cursor_mode        = HK_DEFAULT;
    pointer->drag_scroll        = false;
    pointer->scroll_lock        = HK_FREE;
    pointer->invert_scroll      = false;
    pointer->rounding_carry_x   = 0;
    pointer->rounding_carry_y   = 0;
    pointer->scroll_accum_h     = 0;
    pointer->scroll_accum_v     = 0;

    switch (pointer->kind) {
        case HK_PIMORONI:
            pointer->default_multiplier = 1.5f;
            pointer->sniping_multiplier = 1.0f;
            pointer->scroll_buffer      = 1;
            break;
        case HK_TRACKPOINT:
            pointer->default_multiplier = 2.0f;
            pointer->sniping_multiplier = 1.0f;
            pointer->scroll_buffer      = 5;
            break;
        case HK_AZOTEQ_TPS43:
        case HK_AZOTEQ_TPS65:
            pointer->default_multiplier = 1.25f;
            pointer->sniping_multiplier = 1.0f;
            pointer->scroll_buffer      = 5;
            break;
        default:
            pointer->default_multiplier = 1.0f;
            pointer->sniping_multiplier = 1.0f;
            pointer->scroll_buffer      = 5;
            break;
    }
}

static void hk_save_to_eeprom(void) {
    uint32_t packed = 0;
    packed |= ((uint32_t)HK_EEPROM_CHECK << 24);
    packed |= ((uint32_t)HK_EEPROM_VERSION << 16);
    packed |= ((uint32_t)(g_hk_state.main.drag_scroll ? 1 : 0) << 0);
    packed |= ((uint32_t)(g_hk_state.main.invert_scroll ? 1 : 0) << 1);
    packed |= ((uint32_t)(g_hk_state.peripheral.drag_scroll ? 1 : 0) << 2);
    packed |= ((uint32_t)(g_hk_state.peripheral.invert_scroll ? 1 : 0) << 3);
    eeconfig_update_user_datablock(&packed, 0, sizeof(packed));
}

static bool hk_load_from_eeprom(void) {
    uint32_t packed = 0;
    eeconfig_read_user_datablock(&packed, 0, sizeof(packed));
    uint8_t  check  = (uint8_t)((packed >> 24) & 0xFF);
    uint8_t  ver    = (uint8_t)((packed >> 16) & 0xFF);

    if (check != HK_EEPROM_CHECK || ver != HK_EEPROM_VERSION) {
        return false;
    }

    g_hk_state.main.drag_scroll       = ((packed >> 0) & 0x01) != 0;
    g_hk_state.main.invert_scroll     = ((packed >> 1) & 0x01) != 0;
    g_hk_state.peripheral.drag_scroll = ((packed >> 2) & 0x01) != 0;
    g_hk_state.peripheral.invert_scroll = ((packed >> 3) & 0x01) != 0;
    return true;
}

static float hk_active_multiplier(const hk_pointer_state_t *pointer) {
    return pointer->cursor_mode == HK_SNIPING ? pointer->sniping_multiplier : pointer->default_multiplier;
}

static int8_t hk_clamp_i8(int16_t value) {
    if (value > 127) {
        return 127;
    }
    if (value < -127) {
        return -127;
    }
    return (int8_t)value;
}

static void hk_scale_xy(hk_pointer_state_t *pointer, report_mouse_t *report) {
    float mult = hk_active_multiplier(pointer);

    float sx = ((float)report->x * mult) + pointer->rounding_carry_x;
    float sy = ((float)report->y * mult) + pointer->rounding_carry_y;

    int16_t out_x = (int16_t)lroundf(sx);
    int16_t out_y = (int16_t)lroundf(sy);

    pointer->rounding_carry_x = sx - (float)out_x;
    pointer->rounding_carry_y = sy - (float)out_y;

    report->x = hk_clamp_i8(out_x);
    report->y = hk_clamp_i8(out_y);
}

static void hk_apply_dragscroll(hk_pointer_state_t *pointer, report_mouse_t *report) {
    if (!pointer->drag_scroll) {
        return;
    }

    pointer->scroll_accum_h += report->x;
    pointer->scroll_accum_v += report->y;
    report->x = 0;
    report->y = 0;

    if (pointer->scroll_buffer == 0) {
        pointer->scroll_buffer = 1;
    }

    report->h = hk_clamp_i8(pointer->scroll_accum_h / (int16_t)pointer->scroll_buffer);
    report->v = hk_clamp_i8(pointer->scroll_accum_v / (int16_t)pointer->scroll_buffer);

    pointer->scroll_accum_h -= report->h * (int16_t)pointer->scroll_buffer;
    pointer->scroll_accum_v -= report->v * (int16_t)pointer->scroll_buffer;

    if (pointer->invert_scroll) {
        report->h = -report->h;
        report->v = -report->v;
    }

    if (pointer->scroll_lock == HK_VERTICAL) {
        report->h = 0;
    } else if (pointer->scroll_lock == HK_HORIZONTAL) {
        report->v = 0;
    }
}

static report_mouse_t hk_process_mouse_report(hk_pointer_state_t *pointer, report_mouse_t report) {
    hk_scale_xy(pointer, &report);
    hk_apply_dragscroll(pointer, &report);
    return report;
}

static void hk_init_state(void) {
    g_hk_state.main.is_main_side       = is_keyboard_master();
    g_hk_state.main.kind               = hk_detect_kind_main();
    g_hk_state.peripheral.is_main_side = false;
    g_hk_state.peripheral.kind         = hk_detect_kind_peripheral();

#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) && defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    if (!is_keyboard_master()) {
        g_hk_state.main.kind       = HK_PIMORONI;
        g_hk_state.peripheral.kind = HK_TRACKPOINT;
    } else {
        g_hk_state.main.kind       = HK_TRACKPOINT;
        g_hk_state.peripheral.kind = HK_PIMORONI;
    }
#endif

    hk_set_pointer_defaults(&g_hk_state.main);
    hk_set_pointer_defaults(&g_hk_state.peripheral);
    g_hk_state.dirty = true;
}

void eeconfig_init_user(void) {
    hk_init_state();
    hk_save_to_eeprom();
}

void keyboard_post_init_user(void) {
    hk_init_state();
    g_settings_loaded = hk_load_from_eeprom();
    if (!g_settings_loaded) {
        hk_save_to_eeprom();
    }
    hk_debug_dump_state(&g_hk_state);
}

bool hk_get_dragscroll(bool peripheral) {
    return hk_pointer(peripheral)->drag_scroll;
}

static void hk_cycle_scroll_lock(hk_pointer_state_t *pointer) {
    if (pointer->scroll_lock == HK_FREE) {
        pointer->scroll_lock = HK_VERTICAL;
    } else if (pointer->scroll_lock == HK_VERTICAL) {
        pointer->scroll_lock = HK_HORIZONTAL;
    } else {
        pointer->scroll_lock = HK_FREE;
    }
}

static void hk_adjust_setting(hk_pointer_state_t *pointer, int8_t delta) {
    switch (pointer->cursor_mode) {
        case HK_DEFAULT:
            pointer->default_multiplier += 0.05f * (float)delta;
            if (pointer->default_multiplier < 0.1f) {
                pointer->default_multiplier = 0.1f;
            }
            break;
        case HK_SNIPING:
            pointer->sniping_multiplier += 0.05f * (float)delta;
            if (pointer->sniping_multiplier < 0.1f) {
                pointer->sniping_multiplier = 0.1f;
            }
            break;
        case HK_SCROLL_BUFFER: {
            int16_t next = (int16_t)pointer->scroll_buffer + delta;
            if (next < 1) {
                next = 1;
            }
            if (next > 50) {
                next = 50;
            }
            pointer->scroll_buffer = (uint8_t)next;
            break;
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    g_hk_state.display.key_row      = record->event.key.row;
    g_hk_state.display.key_col      = record->event.key.col;
    g_hk_state.display.key_pressed  = record->event.pressed;
    g_hk_state.display.highest_layer = get_highest_layer(layer_state);

    bool                peripheral = get_mods() & MOD_MASK_SHIFT;
    hk_pointer_state_t *pointer    = hk_pointer(peripheral);

    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case HK_SAVE:
            hk_save_to_eeprom();
            return false;
        case HK_RESET:
            hk_init_state();
            hk_save_to_eeprom();
            return false;
        case HK_DUMP:
            hk_debug_dump_state(&g_hk_state);
            return false;
        case HK_P_SET_DEFAULT:
            pointer->cursor_mode = HK_DEFAULT;
            return false;
        case HK_P_SET_SNIPING:
            pointer->cursor_mode = HK_SNIPING;
            return false;
        case HK_P_SET_SCROLL_BUFFER:
            pointer->cursor_mode = HK_SCROLL_BUFFER;
            return false;
        case HK_S_MODE:
            pointer->cursor_mode = HK_SNIPING;
            return false;
        case HK_S_MODE_T:
            pointer->cursor_mode = pointer->cursor_mode == HK_SNIPING ? HK_DEFAULT : HK_SNIPING;
            return false;
        case HK_D_MODE:
            pointer->drag_scroll = true;
            return false;
        case HK_D_MODE_T:
            pointer->drag_scroll = !pointer->drag_scroll;
            return false;
        case HK_C_SCROLL:
            hk_cycle_scroll_lock(pointer);
            return false;
        case HK_I_SCROLL:
            pointer->invert_scroll = !pointer->invert_scroll;
            return false;
        case KC_UP:
            hk_adjust_setting(pointer, +1);
            return true;
        case KC_DOWN:
            hk_adjust_setting(pointer, -1);
            return true;
        default:
            return true;
    }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    g_hk_state.last_mouse_report = mouse_report;
    return hk_process_mouse_report(&g_hk_state.main, mouse_report);
}

#ifdef POINTING_DEVICE_COMBINED
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) && defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    report_mouse_t main_report = is_keyboard_master() ? right_report : left_report;
    report_mouse_t peri_report = is_keyboard_master() ? left_report : right_report;
#else
    report_mouse_t main_report = left_report;
    report_mouse_t peri_report = right_report;
#endif

    report_mouse_t main_out = hk_process_mouse_report(&g_hk_state.main, main_report);
    report_mouse_t peri_out = hk_process_mouse_report(&g_hk_state.peripheral, peri_report);

    report_mouse_t out = {
        .x       = hk_clamp_i8((int16_t)main_out.x + (int16_t)peri_out.x),
        .y       = hk_clamp_i8((int16_t)main_out.y + (int16_t)peri_out.y),
        .h       = hk_clamp_i8((int16_t)main_out.h + (int16_t)peri_out.h),
        .v       = hk_clamp_i8((int16_t)main_out.v + (int16_t)peri_out.v),
        .buttons = main_out.buttons | peri_out.buttons,
    };
    g_hk_state.last_mouse_report = out;
    return out;
}
#endif
