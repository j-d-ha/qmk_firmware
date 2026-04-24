#pragma once

#define MASTER_RIGHT

#if !defined(PS2_DATA_PIN)
#    if defined(__AVR__)
#        define PS2_DATA_PIN D1
#    else
#        define PS2_DATA_PIN GP2
#    endif
#endif

#if !defined(PS2_CLOCK_PIN)
#    if defined(__AVR__)
#        define PS2_CLOCK_PIN D0
#    else
#        define PS2_CLOCK_PIN GP3
#    endif
#endif

#define PS2_MOUSE_USE_REMOTE_MODE
#define PS2_MOUSE_INIT_DELAY 500
#define PS2_MOUSE_INVERT_X
#define PS2_MOUSE_INVERT_Y
