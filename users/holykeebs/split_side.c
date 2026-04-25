#include "split_util.h"

#if defined(HK_BUILD_SIDE_LEFT)

bool is_keyboard_left_impl(void) {
    return true;
}

#elif defined(HK_BUILD_SIDE_RIGHT)

bool is_keyboard_left_impl(void) {
    return false;
}

#endif
