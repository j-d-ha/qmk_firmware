#pragma once

#if defined(SPLIT_KEYBOARD)
#    if defined(HK_MASTER_LEFT)
#        undef MASTER_LEFT
#        define MASTER_LEFT
#    elif defined(HK_MASTER_RIGHT)
#        undef MASTER_RIGHT
#        define MASTER_RIGHT
#    else
#        error "Missing HK master side definition"
#    endif

#    if !defined(__AVR__)
#        undef SERIAL_USART_TX_PIN
#        define SERIAL_USART_TX_PIN GP1
#    endif
#endif

#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) || defined(HK_POINTING_DEVICE_RIGHT_PIMORONI)
#    ifdef HK_POINTING_DEVICE_RIGHT_PIMORONI
#        ifdef POINTING_DEVICE_COMBINED
#            define POINTING_DEVICE_ROTATION_90_RIGHT
#        else
#            define POINTING_DEVICE_ROTATION_90
#        endif
#    endif
#    ifdef HK_POINTING_DEVICE_LEFT_PIMORONI
#        define POINTING_DEVICE_ROTATION_270
#    endif
#endif

#if defined(HK_POINTING_DEVICE_LEFT_TRACKPOINT) || defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
#    ifndef PS2_MOUSE_USE_REMOTE_MODE
#        define PS2_MOUSE_USE_REMOTE_MODE
#    endif
#    ifndef PS2_MOUSE_INVERT_X
#        define PS2_MOUSE_INVERT_X
#    endif
#    ifndef PS2_MOUSE_INVERT_Y
#        define PS2_MOUSE_INVERT_Y
#    endif
#    ifndef PS2_MOUSE_INIT_DELAY
#        define PS2_MOUSE_INIT_DELAY 500
#    endif
#    ifndef PS2_DATA_PIN
#        if defined(__AVR__)
#            define PS2_DATA_PIN D1
#        else
#            define PS2_DATA_PIN GP2
#        endif
#    endif
#    ifndef PS2_CLOCK_PIN
#        if defined(__AVR__)
#            define PS2_CLOCK_PIN D0
#        else
#            define PS2_CLOCK_PIN GP3
#        endif
#    endif
#    if !defined(__AVR__) && !defined(PS2_PIO_USE_PIO1)
#        define PS2_PIO_USE_PIO1
#    endif
#endif

#if defined(HK_POINTING_DEVICE_LEFT_PIMORONI) && defined(HK_POINTING_DEVICE_RIGHT_TRACKPOINT)
#    ifndef POINTING_DEVICE_TASK_THROTTLE_MS
#        define POINTING_DEVICE_TASK_THROTTLE_MS 1
#    endif
#endif

#ifndef EECONFIG_USER_DATA_SIZE
#    define EECONFIG_USER_DATA_SIZE 64
#endif
