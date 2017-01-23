
#include "main.h"

// Returns true only once per period_ms. cur_time is the current time in ms. prev_period is a pointer to uint32_t that
// keeps data between calls, should not be used otherwise; slips will contain the number of skipped periods, if provided.
// Usage:
// void loop() {
//     static uint32_t block_period = 0;
//     if (throttle_ms(1000, millis(), &block_period)) {
//         // This block will be executed once every 1000 ms.
//     }   
// }
bool throttle_ms(uint32_t period_ms, uint32_t cur_time, uint32_t *prev_period, uint32_t *slips) {
    uint32_t cur_period = cur_time / period_ms;
    if (cur_period == *prev_period) {
        return false;  // We're in the same period
    } else {
        if (slips && *prev_period && cur_period != *prev_period+1)
            (*slips)++;
        *prev_period = cur_period;
        return true;
    }
}
