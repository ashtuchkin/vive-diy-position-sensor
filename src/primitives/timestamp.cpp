#include "primitives/timestamp.h"

uint32_t Timestamp::get_value(TimeUnit tu) const {
    // To "extend" the period we assume that the timestamp is for close enough point in time to now.
    // Then, use millis and estimate the number of overflows happened.
    uint32_t cur_millis = Timestamp::cur_time_millis();
    Timestamp ts_cur_millis(cur_millis * msec);  // This will overflow, but that's what we want.
    int32_t delta_in_tu = (*this - ts_cur_millis).get_value(tu);

    uint32_t cur_time_in_tu = 0;
    if (tu == msec) {
        cur_time_in_tu = cur_millis;
    } else if (tu > msec && (tu % msec == 0)) {
        cur_time_in_tu = cur_millis / (tu / msec);
    } else if (tu < msec && (msec % tu == 0)) {
        cur_time_in_tu = cur_millis * (msec / tu);
    } else {
        cur_time_in_tu = cur_millis * ((float)msec / tu);
    }
    return cur_time_in_tu + delta_in_tu;
}
