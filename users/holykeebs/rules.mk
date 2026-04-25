TRI_LAYER_ENABLE = yes
EXTRAKEY_ENABLE  = yes
MOUSEKEY_ENABLE  = yes

ifneq ($(POINTING_DEVICE),)
    ifeq ($(PLATFORM),CHIBIOS)
        SERIAL_DRIVER = vendor
    endif

    SRC += $(USER_PATH)/holykeebs.c
    SRC += $(USER_PATH)/hk_debug.c
    SRC += $(USER_PATH)/pimoroni.c
    SRC += $(USER_PATH)/trackpoint.c
    SRC += $(USER_PATH)/split_side.c

    VALID_POINTING_DEVICES := trackball trackpoint trackball_trackpoint trackpoint_trackball
    ifeq ($(filter $(POINTING_DEVICE),$(VALID_POINTING_DEVICES)),)
        $(error [HolyKeebs] Invalid POINTING_DEVICE='$(POINTING_DEVICE)'. Valid: $(VALID_POINTING_DEVICES))
    endif

    ifeq ($(POINTING_DEVICE),trackball)
        POINTING_DEVICE_ENABLE = yes
        POINTING_DEVICE_DRIVER = pimoroni_trackball
        OPT_DEFS += -DHK_POINTING_DEVICE_RIGHT_PIMORONI
        OPT_DEFS += -DHK_MASTER_RIGHT
    endif

    ifeq ($(POINTING_DEVICE),trackpoint)
        POINTING_DEVICE_ENABLE = yes
        POINTING_DEVICE_DRIVER = ps2
        ifeq ($(PLATFORM),CHIBIOS)
            PS2_DRIVER ?= vendor
        else
            PS2_DRIVER ?= busywait
        endif
        OPT_DEFS += -DHK_POINTING_DEVICE_RIGHT_TRACKPOINT
        OPT_DEFS += -DHK_MASTER_RIGHT
    endif

    ifneq ($(filter $(POINTING_DEVICE),trackball_trackpoint trackpoint_trackball),)
        POINTING_DEVICE_ENABLE = yes
        SPLIT_POINTING_ENABLE  = yes
        POINTING_DEVICE_COMBINED = yes
        OPT_DEFS += -DSPLIT_POINTING_ENABLE
        OPT_DEFS += -DPOINTING_DEVICE_COMBINED
        SIDE ?= right

        OPT_DEFS += -DHK_POINTING_DEVICE_LEFT_PIMORONI
        OPT_DEFS += -DHK_POINTING_DEVICE_RIGHT_TRACKPOINT
        OPT_DEFS += -DHK_MASTER_RIGHT

        ifeq ($(SIDE),left)
            POINTING_DEVICE_DRIVER = pimoroni_trackball
            OPT_DEFS += -DHK_BUILD_SIDE_LEFT
        else ifeq ($(SIDE),right)
            POINTING_DEVICE_DRIVER = ps2
            OPT_DEFS += -DHK_BUILD_SIDE_RIGHT
            ifeq ($(PLATFORM),CHIBIOS)
                PS2_DRIVER ?= vendor
            else
                PS2_DRIVER ?= busywait
            endif
        else
            $(error [HolyKeebs] SIDE must be left or right)
        endif
    endif
endif

ifeq ($(OLED),yes)
    OLED_ENABLE = yes
    OPT_DEFS += -DHK_OLED_ENABLE
else ifeq ($(OLED),stock)
    OLED_ENABLE = yes
else
    OLED_ENABLE = no
endif
