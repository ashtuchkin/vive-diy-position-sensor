#include "led_state.h"
#include "primitives/timestamp.h"
#include "primitives/string_utils.h"
#include "input.h"
#include <stdarg.h>
#include <stdio.h>

void set_led_state(LedState state) {
    // Do nothing
}
void update_led_pattern(Timestamp cur_time) {
    
}

Timestamp Timestamp::cur_time() {
    return 0;
}

uint32_t Timestamp::cur_time_millis() {
    return 0;
}
