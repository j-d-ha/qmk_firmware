#include "holykeebs.h"

#include <math.h>
#include <string.h>

#include "action_layer.h"
#include "action_util.h"
#include "eeconfig.h"
#include "eeprom_config.h"
#include "hk_debug.h"

hk_state_t g_hk_state = {0};

__attribute__((weak)) bool process_record_keymap(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    return true;
}

__attribute__((weak)) report_mouse_t pointing_device_task_keymap(report_mouse_t mouse_report) {
    return mouse_report;
}

#ifdef POINTING_DEVICE_COMBINED
__attribute__((weak)) report_mouse_t pointing_device_task_combined_keymap(report_mouse_t mouse_report) {
    return mouse_report;
}
#endif

static bool hk_has_shift_mod(void) {
    return (get_mods() & MOD_MASK_SHIFT) || (get_oneshot_mods() & MOD_MASK_SHIFT);
}

static hk_pointer_state_t *hk_pointer(bool peripheral) {
    return peripheral ? &g_hk_state.peripheral : &g_hk_state.main;
}

static hk_pointing_device_type hk_kind_for_right(void) {
#if defined(HK_POINTING_DEVICE_RIGHT_PIMORONI)
    return HK_PIMORONI;
#elif defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
    return HK_TRACKPOINT;
#elif defined(HK_POINTING_DEVICE_RIGHT_CIRQUE35)
    return HK_CIRQUE35;
#elif defined(HK_POINTING_DEVICE_RIGHT_CIRQUE40)
    return HK_CIRQUE40;
#elif defined(HK_POINTING_DEVICE_RIGHT_TPS43)
    return HK_AZOTEQ_TPS43;
#else
    return HK_NONE;
#endif
}

static hk_pointing_device_type hk_kind_for_left(void) {
#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI)
    return HK_PIMORONI;
#elif defined(HK_POINTING_DEVICE_LEFT_TRACKPOINT)
    return HK_TRACKPOINT;
#elif defined(HK_POINTING_DEVICE_LEFT_CIRQUE35)
    return HK_CIRQUE35;
#elif defined(HK_POINTING_DEVICE_LEFT_CIRQUE40)
    return HK_CIRQUE40;
#elif defined(HK_POINTING_DEVICE_LEFT_TPS43)
    return HK_AZOTEQ_TPS43;
#elif defined(HK_POINTING_DEVICE_MIDDLE_TPS65)
    return HK_AZOTEQ_TPS65;
#else
    return HK_NONE;
#endif
}

static void hk_set_pointer_defaults(hk_pointer_state_t *pointer) {
    pointer->cursor_mode      = HK_DEFAULT;
    pointer->drag_scroll      = false;
    pointer->scroll_lock      = HK_FREE;
    pointer->invert_scroll    = false;
    pointer->rounding_carry_x = 0;
    pointer->rounding_carry_y = 0;
    pointer->scroll_accum_h   = 0;
    pointer->scroll_accum_v   = 0;

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

static uint16_t hk_to_x100(float value) {
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 655.35f) {
        return 65535;
    }
    return (uint16_t)lroundf(value * 100.0f);
}

static float hk_from_x100(uint16_t value) {
    return ((float)value) / 100.0f;
}

static void hk_save_to_eeprom(void) {
    hk_eeprom_config_t cfg = {
        .version = HK_EEPROM_VERSION,
        .check   = HK_EEPROM_CHECK,
        .main    = {
            .cursor_mode              = g_hk_state.main.cursor_mode,
            .drag_scroll              = g_hk_state.main.drag_scroll,
            .scroll_lock              = g_hk_state.main.scroll_lock,
            .invert_scroll            = g_hk_state.main.invert_scroll,
            .default_multiplier_x100  = hk_to_x100(g_hk_state.main.default_multiplier),
            .sniping_multiplier_x100  = hk_to_x100(g_hk_state.main.sniping_multiplier),
            .scroll_buffer            = g_hk_state.main.scroll_buffer,
        },
        .peripheral = {
            .cursor_mode              = g_hk_state.peripheral.cursor_mode,
            .drag_scroll              = g_hk_state.peripheral.drag_scroll,
            .scroll_lock              = g_hk_state.peripheral.scroll_lock,
            .invert_scroll            = g_hk_state.peripheral.invert_scroll,
            .default_multiplier_x100  = hk_to_x100(g_hk_state.peripheral.default_multiplier),
            .sniping_multiplier_x100  = hk_to_x100(g_hk_state.peripheral.sniping_multiplier),
            .scroll_buffer            = g_hk_state.peripheral.scroll_buffer,
        },
    };

    eeconfig_update_user_datablock(&cfg, 0, sizeof(cfg));
}

static bool hk_load_from_eeprom(void) {
    hk_eeprom_config_t cfg = {0};
    eeconfig_read_user_datablock(&cfg, 0, sizeof(cfg));

    if (cfg.check != HK_EEPROM_CHECK || cfg.version != HK_EEPROM_VERSION) {
        return false;
    }

    g_hk_state.main.cursor_mode         = cfg.main.cursor_mode;
    g_hk_state.main.drag_scroll         = cfg.main.drag_scroll;
    g_hk_state.main.scroll_lock         = cfg.main.scroll_lock;
    g_hk_state.main.invert_scroll       = cfg.main.invert_scroll;
    g_hk_state.main.default_multiplier  = hk_from_x100(cfg.main.default_multiplier_x100);
    g_hk_state.main.sniping_multiplier  = hk_from_x100(cfg.main.sniping_multiplier_x100);
    g_hk_state.main.scroll_buffer       = cfg.main.scroll_buffer ? cfg.main.scroll_buffer : 1;

    g_hk_state.peripheral.cursor_mode        = cfg.peripheral.cursor_mode;
    g_hk_state.peripheral.drag_scroll        = cfg.peripheral.drag_scroll;
    g_hk_state.peripheral.scroll_lock        = cfg.peripheral.scroll_lock;
    g_hk_state.peripheral.invert_scroll      = cfg.peripheral.invert_scroll;
    g_hk_state.peripheral.default_multiplier = hk_from_x100(cfg.peripheral.default_multiplier_x100);
    g_hk_state.peripheral.sniping_multiplier = hk_from_x100(cfg.peripheral.sniping_multiplier_x100);
    g_hk_state.peripheral.scroll_buffer      = cfg.peripheral.scroll_buffer ? cfg.peripheral.scroll_buffer : 1;

    return true;
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

static float hk_active_multiplier(const hk_pointer_state_t *pointer) {
    return pointer->cursor_mode == HK_SNIPING ? pointer->sniping_multiplier : pointer->default_multiplier;
}

static void hk_scale_xy(hk_pointer_state_t *pointer, report_mouse_t *report) {
    float sx = ((float)report->x * hk_active_multiplier(pointer)) + pointer->rounding_carry_x;
    float sy = ((float)report->y * hk_active_multiplier(pointer)) + pointer->rounding_carry_y;

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

static void hk_process_mouse_report(hk_pointer_state_t *pointer, report_mouse_t *report) {
    hk_scale_xy(pointer, report);
    hk_apply_dragscroll(pointer, report);
}

#ifdef POINTING_DEVICE_COMBINED
static report_mouse_t hk_combine_reports(report_mouse_t a, report_mouse_t b) {
    report_mouse_t out = {
        .x       = hk_clamp_i8((int16_t)a.x + (int16_t)b.x),
        .y       = hk_clamp_i8((int16_t)a.y + (int16_t)b.y),
        .h       = hk_clamp_i8((int16_t)a.h + (int16_t)b.h),
        .v       = hk_clamp_i8((int16_t)a.v + (int16_t)b.v),
        .buttons = a.buttons | b.buttons,
    };
    return out;
}
#endif

static void hk_init_state(void) {
    memset(&g_hk_state, 0, sizeof(g_hk_state));

    g_hk_state.init         = true;
    g_hk_state.is_main_side = is_keyboard_master();

    if (!g_hk_state.is_main_side) {
        return;
    }

    hk_pointing_device_type right_kind = hk_kind_for_right();
    hk_pointing_device_type left_kind  = hk_kind_for_left();

    g_hk_state.main.kind               = right_kind;
    g_hk_state.main.is_main_side       = true;
    g_hk_state.peripheral.kind         = left_kind;
    g_hk_state.peripheral.is_main_side = false;

#if defined(SPLIT_KEYBOARD) && !defined(HK_POINTING_DEVICE_MIDDLE_TPS65)
    if (is_keyboard_left()) {
        hk_pointing_device_type tmp = g_hk_state.main.kind;
        g_hk_state.main.kind        = g_hk_state.peripheral.kind;
        g_hk_state.peripheral.kind  = tmp;
    }
#endif

    hk_set_pointer_defaults(&g_hk_state.main);
    hk_set_pointer_defaults(&g_hk_state.peripheral);

#ifdef HK_MAIN_DEFAULT_POINTER_DEFAULT_MULTIPLIER
    g_hk_state.main.default_multiplier = HK_MAIN_DEFAULT_POINTER_DEFAULT_MULTIPLIER;
#endif
#ifdef HK_MAIN_DEFAULT_POINTER_SNIPING_MULTIPLIER
    g_hk_state.main.sniping_multiplier = HK_MAIN_DEFAULT_POINTER_SNIPING_MULTIPLIER;
#endif
#ifdef HK_MAIN_DEFAULT_POINTER_SCROLL_BUFFER
    g_hk_state.main.scroll_buffer = HK_MAIN_DEFAULT_POINTER_SCROLL_BUFFER;
#endif
#ifdef HK_PERIPHERAL_DEFAULT_POINTER_DEFAULT_MULTIPLIER
    g_hk_state.peripheral.default_multiplier = HK_PERIPHERAL_DEFAULT_POINTER_DEFAULT_MULTIPLIER;
#endif
#ifdef HK_PERIPHERAL_DEFAULT_POINTER_SNIPING_MULTIPLIER
    g_hk_state.peripheral.sniping_multiplier = HK_PERIPHERAL_DEFAULT_POINTER_SNIPING_MULTIPLIER;
#endif
#ifdef HK_PERIPHERAL_DEFAULT_POINTER_SCROLL_BUFFER
    g_hk_state.peripheral.scroll_buffer = HK_PERIPHERAL_DEFAULT_POINTER_SCROLL_BUFFER;
#endif

    if (g_hk_state.peripheral.kind == HK_PIMORONI) {
        g_hk_state.peripheral.drag_scroll = true;
    }

    g_hk_state.dirty = true;
}

void eeconfig_init_user(void) {
    hk_init_state();
    if (g_hk_state.is_main_side) {
        hk_save_to_eeprom();
    }
}

void keyboard_post_init_user(void) {
    hk_init_state();

    if (!g_hk_state.is_main_side) {
        return;
    }

    if (!hk_load_from_eeprom()) {
        hk_save_to_eeprom();
    }

    hk_debug_dump_state(&g_hk_state);
}

bool hk_get_dragscroll(bool peripheral) {
    return hk_pointer(peripheral)->drag_scroll;
}

static void hk_cycle_scroll_lock(hk_pointer_state_t *pointer) {
    switch (pointer->scroll_lock) {
        case HK_FREE:
            pointer->scroll_lock = HK_VERTICAL;
            break;
        case HK_VERTICAL:
            pointer->scroll_lock = HK_HORIZONTAL;
            break;
        default:
            pointer->scroll_lock = HK_FREE;
            break;
    }
}

static void hk_adjust_multiplier(float *value, int8_t delta) {
    *value += 0.05f * (float)delta;
    if (*value < 0.1f) {
        *value = 0.1f;
    }
}

static void hk_adjust_scroll_buffer(uint8_t *value, int8_t delta) {
    int16_t next = (int16_t)(*value) + delta;
    if (next < 1) {
        next = 1;
    }
    if (next > 50) {
        next = 50;
    }
    *value = (uint8_t)next;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    bool propagate_event = true;
    bool state_changed   = false;

    g_hk_state.display.key_row       = record->event.key.row;
    g_hk_state.display.key_col       = record->event.key.col;
    g_hk_state.display.key_pressed   = record->event.pressed;
    g_hk_state.display.highest_layer = get_highest_layer(layer_state);

    if (!g_hk_state.is_main_side) {
        return process_record_keymap(keycode, record);
    }

    propagate_event = process_record_keymap(keycode, record);
    if (!propagate_event) {
        return false;
    }

    hk_pointer_state_t *pointer = hk_pointer(hk_has_shift_mod());

    switch (keycode) {
        case HK_SAVE:
            if (record->event.pressed) {
                hk_save_to_eeprom();
            }
            propagate_event = false;
            break;
        case HK_RESET:
            if (record->event.pressed) {
                hk_init_state();
                hk_save_to_eeprom();
                state_changed = true;
            }
            propagate_event = false;
            break;
        case HK_DUMP:
            if (record->event.pressed) {
                hk_debug_dump_state(&g_hk_state);
            }
            propagate_event = false;
            break;
        case HK_P_SET_DEFAULT:
            g_hk_state.setting_default_scale = record->event.pressed;
            propagate_event                  = false;
            break;
        case HK_P_SET_SNIPING:
            g_hk_state.setting_sniping_scale = record->event.pressed;
            propagate_event                  = false;
            break;
        case HK_P_SET_SCROLL_BUFFER:
            g_hk_state.setting_scroll_buffer = record->event.pressed;
            propagate_event                  = false;
            break;
        case HK_S_MODE:
            pointer->cursor_mode = record->event.pressed ? HK_SNIPING : HK_DEFAULT;
            state_changed        = true;
            propagate_event      = false;
            break;
        case HK_S_MODE_T:
            if (record->event.pressed) {
                pointer->cursor_mode = pointer->cursor_mode == HK_SNIPING ? HK_DEFAULT : HK_SNIPING;
                state_changed        = true;
            }
            propagate_event = false;
            break;
        case HK_D_MODE:
            pointer->drag_scroll = record->event.pressed;
            state_changed        = true;
            propagate_event      = false;
            break;
        case HK_D_MODE_T:
            if (record->event.pressed) {
                pointer->drag_scroll = !pointer->drag_scroll;
                state_changed        = true;
            }
            propagate_event = false;
            break;
        case HK_C_SCROLL:
            if (record->event.pressed) {
                hk_cycle_scroll_lock(pointer);
                state_changed = true;
            }
            propagate_event = false;
            break;
        case HK_I_SCROLL:
            if (record->event.pressed) {
                pointer->invert_scroll = !pointer->invert_scroll;
                state_changed          = true;
            }
            propagate_event = false;
            break;
        case KC_UP:
        case KC_DOWN:
            if (record->event.pressed) {
                int8_t delta = (keycode == KC_UP) ? 1 : -1;
                if (g_hk_state.setting_default_scale) {
                    hk_adjust_multiplier(&pointer->default_multiplier, delta);
                    state_changed = true;
                } else if (g_hk_state.setting_sniping_scale) {
                    hk_adjust_multiplier(&pointer->sniping_multiplier, delta);
                    state_changed = true;
                } else if (g_hk_state.setting_scroll_buffer) {
                    hk_adjust_scroll_buffer(&pointer->scroll_buffer, delta);
                    state_changed = true;
                }
            }
            if (g_hk_state.setting_default_scale || g_hk_state.setting_sniping_scale || g_hk_state.setting_scroll_buffer) {
                propagate_event = false;
            }
            break;
        default:
            break;
    }

    if (state_changed) {
        g_hk_state.dirty = true;
    }

    return propagate_event;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (!g_hk_state.init) {
        hk_init_state();
    }

    if (!g_hk_state.is_main_side) {
        return pointing_device_task_keymap(mouse_report);
    }

    hk_process_mouse_report(&g_hk_state.main, &mouse_report);
    g_hk_state.last_mouse_report = mouse_report;
    return pointing_device_task_keymap(mouse_report);
}

#ifdef POINTING_DEVICE_COMBINED
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    if (!g_hk_state.init) {
        hk_init_state();
    }

    if (!g_hk_state.is_main_side) {
        return pointing_device_task_combined_keymap(hk_combine_reports(left_report, right_report));
    }

    report_mouse_t *main_report = is_keyboard_left() ? &left_report : &right_report;
    report_mouse_t *peri_report = is_keyboard_left() ? &right_report : &left_report;

    hk_process_mouse_report(&g_hk_state.main, main_report);
    hk_process_mouse_report(&g_hk_state.peripheral, peri_report);

    report_mouse_t out       = hk_combine_reports(left_report, right_report);
    g_hk_state.last_mouse_report = out;
    return pointing_device_task_combined_keymap(out);
}
#endif
