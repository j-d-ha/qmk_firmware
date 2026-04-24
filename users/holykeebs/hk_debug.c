#include "hk_debug.h"

#include "print.h"

void hk_debug_dump_state(const hk_state_t *state) {
#ifdef CONSOLE_ENABLE
    if (!state) {
        return;
    }

    xprintf("HK main kind=%u mode=%u drag=%u lock=%u inv=%u def=%.2f sni=%.2f buf=%u\n",
            state->main.kind,
            state->main.cursor_mode,
            state->main.drag_scroll,
            state->main.scroll_lock,
            state->main.invert_scroll,
            state->main.default_multiplier,
            state->main.sniping_multiplier,
            state->main.scroll_buffer);
    xprintf("HK peri kind=%u mode=%u drag=%u lock=%u inv=%u def=%.2f sni=%.2f buf=%u\n",
            state->peripheral.kind,
            state->peripheral.cursor_mode,
            state->peripheral.drag_scroll,
            state->peripheral.scroll_lock,
            state->peripheral.invert_scroll,
            state->peripheral.default_multiplier,
            state->peripheral.sniping_multiplier,
            state->peripheral.scroll_buffer);
#else
    (void)state;
#endif
}
