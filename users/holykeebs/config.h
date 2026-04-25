#pragma once

/* Reset. */
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 1000U

#define MOUSE_EXTENDED_REPORT
#define WHEEL_EXTENDED_REPORT

#undef PRINTF_SUPPORT_DECIMAL_SPECIFIERS
#define PRINTF_SUPPORT_DECIMAL_SPECIFIERS 1

#define EECONFIG_USER_DATA_SIZE 64

#undef MASTER_LEFT
#undef EE_HANDS
#undef MASTER_RIGHT

#if defined(HK_MASTER_LEFT)
    #define MASTER_LEFT
#elif defined(HK_MASTER_RIGHT)
    #define MASTER_RIGHT
#else
    #error "HK_MASTER_LEFT or HK_MASTER_RIGHT not defined in rules.mk"
#endif

#ifdef SPLIT_KEYBOARD
    #define SERIAL_USART_TX_PIN GP1

    // This helps in setups where the keyboard isn't recognized on boot if it's already plugged in.
    #define SPLIT_WATCHDOG_ENABLE
    #define SPLIT_WATCHDOG_TIMEOUT 3000
#endif

#ifdef HK_POINTING_DEVICE_RIGHT_PIMORONI
    #ifdef POINTING_DEVICE_COMBINED
        #define POINTING_DEVICE_ROTATION_90_RIGHT
    #else
        #define POINTING_DEVICE_ROTATION_90
    #endif
#endif
#ifdef HK_POINTING_DEVICE_LEFT_PIMORONI
    #define POINTING_DEVICE_ROTATION_270
#endif

#if defined(HK_POINTING_DEVICE_RIGHT_PIMORONI) || defined(HK_POINTING_DEVICE_LEFT_PIMORONI)
    #if defined(POINTING_DEVICE_DRIVER_azoteq_iqs5xx) || defined(POINTING_DEVICE_DRIVER_cirque_pinnacle_i2c)
        #define POINTING_DEVICE_TASK_THROTTLE_MS 1
    #endif
#endif

#ifdef HK_POINTING_DEVICE_LEFT_CIRQUE40
    #define POINTING_DEVICE_ROTATION_180
#endif

#ifdef POINTING_DEVICE_DRIVER_cirque_pinnacle_i2c
    #if defined(HK_POINTING_DEVICE_LEFT_CIRQUE35) || defined(HK_POINTING_DEVICE_RIGHT_CIRQUE35)
        #define CIRQUE_PINNACLE_DIAMETER_MM 35
    #elif defined(HK_POINTING_DEVICE_LEFT_CIRQUE40) || defined(HK_POINTING_DEVICE_RIGHT_CIRQUE40)
        #define CIRQUE_PINNACLE_DIAMETER_MM 40
    #else
        #error "Unknown Cirque configuration."
    #endif

    #define CIRQUE_PINNACLE_TAP_ENABLE
    #define POINTING_DEVICE_GESTURES_SCROLL_ENABLE
#endif

#ifdef POINTING_DEVICE_DRIVER_azoteq_iqs5xx
    #if defined(HK_POINTING_DEVICE_RIGHT_TPS43) || defined(HK_POINTING_DEVICE_LEFT_TPS43)
        #define AZOTEQ_IQS5XX_TPS43
    #endif

    #if defined(HK_POINTING_DEVICE_RIGHT_TPS43)
        #if !defined(HK_POINTING_DEVICE_LEFT_TPS43)
            #define AZOTEQ_IQS5XX_ROTATION_180
        #endif
    #endif

    #define AZOTEQ_IQS5XX_PRESS_AND_HOLD_ENABLE true
#endif

#ifdef POINTING_DEVICE_DRIVER_ps2
    #define PS2_MOUSE_USE_REMOTE_MODE
    #define PS2_PIO_USE_PIO1
    #define PS2_MOUSE_INVERT_X
    #define PS2_MOUSE_INVERT_Y
    #define PS2_DATA_PIN    GP2
    #define PS2_CLOCK_PIN   GP3
    #define PS2_MOUSE_INIT_DELAY 500
#endif

#if defined(OLED_ENABLE) && defined(SPLIT_KEYBOARD)
    #define HK_SPLIT_SYNC_STATE
#endif

#ifdef HK_OLED_ENABLE
#    define OLED_FONT_H "users/holykeebs/logofont/logofont.c"
#    define OLED_FONT_START 32
#    define OLED_FONT_END 195
#endif

#ifdef HK_SPLIT_SYNC_STATE
    #define SPLIT_LAYER_STATE_ENABLE
    #define SPLIT_TRANSACTION_IDS_USER HK_SYNC_STATE
    #define RPC_M2S_BUFFER_SIZE 64
#endif
