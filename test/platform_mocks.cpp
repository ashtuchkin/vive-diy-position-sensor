#include "led_state.h"
#include "primitives/timestamp.h"
#include "primitives/string_utils.h"
#include "input.h"
#include <stdarg.h>
#include <stdio.h>
#include <Print.h>

void set_led_state(LedState state) {
    // Do nothing
}

Timestamp Timestamp::cur_time() {
    return 0;
}

uint32_t Timestamp::get_value(TimeUnit tu) const {
    // TODO: Make platform-independent.
    return 0;
}

size_t Print::write(const uint8_t *buffer, size_t size) {
    // Do nothing
    return 0;
}

int Print::printf(const char *format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    int res = vsnprintf(buf, sizeof(buf), format, args);
    if (res > 0) {
        write(buf, res);
    }
    va_end(args);
    return res;
}

std::unique_ptr<InputNode> createInputCmpNode(uint32_t input_idx, const InputDef &input_def) {
    return nullptr;
}


