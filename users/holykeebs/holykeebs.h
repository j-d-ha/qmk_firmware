#pragma once

#include <stdbool.h>

#include "quantum.h"
#include "pointing.h"

enum custom_keycodes {
    HK_SAVE = SAFE_RANGE,
    HK_RESET,
    HK_DUMP,
    HK_P_SET_DEFAULT,
    HK_P_SET_SNIPING,
    HK_P_SET_SCROLL_BUFFER,
    HK_S_MODE,
    HK_S_MODE_T,
    HK_D_MODE,
    HK_D_MODE_T,
    HK_C_SCROLL,
    HK_I_SCROLL,
};

extern hk_state_t g_hk_state;

bool hk_get_dragscroll(bool peripheral);
