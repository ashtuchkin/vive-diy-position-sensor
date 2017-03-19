#pragma once
#include "primitives/timestamp.h"
// This file is used to set current led blinking pattern/state.
// To update led state, call set_led_state().
// In normal operation, call update_led_state() frequently.

enum class LedState {
    kNotInitialized,
    kConfigMode,
    kNoFix,
    kFixFound,
};

void set_led_state(LedState state);
void update_led_pattern(Timestamp cur_time);

