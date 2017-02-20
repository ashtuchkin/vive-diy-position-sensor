#include "led_state.h"
#include <Arduino.h>

struct LedPattern {
    TimeDelta period;
    const char *pattern;
} patterns[] = {
    [(int)LedState::kNotInitialized] = {TimeDelta(1000, ms), "0"},
    [(int)LedState::kConfigMode]     = {TimeDelta(200, ms), "10100000"},
    [(int)LedState::kNoFix]          = {TimeDelta(500, ms), "10"},
    [(int)LedState::kFixFound]       = {TimeDelta(30, ms),  "10"},
};

static bool initialized = false;
static uint32_t pattern_idx = 0;
static LedState cur_state = LedState::kNotInitialized;
static Timestamp prev_called;  // LongTimestamp

void set_led_state(LedState state) {
    if (state != cur_state) {
        cur_state = state;
        pattern_idx = 0;
    }
}

void update_led_pattern(Timestamp cur_time) {
    if (!initialized) {
        pinMode(LED_BUILTIN, OUTPUT);
        initialized = true;    
    }

    LedPattern &pat = patterns[(int)cur_state];
    if (throttle_ms(pat.period, cur_time, &prev_called)) {
        bool new_led_state = pat.pattern[pattern_idx++] == '1';
        if (pat.pattern[pattern_idx] == 0) pattern_idx = 0;

        digitalWriteFast(LED_BUILTIN, new_led_state);
    }
}
