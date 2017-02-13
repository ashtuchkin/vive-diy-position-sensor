#include "timestamp.h"
#include <Arduino.h>

Timestamp Timestamp::cur_time() {
    // Repeat logic inside micros(), but get better precision, up to a single CPU tick.
    
    extern volatile uint32_t systick_millis_count;
	__disable_irq();
	uint32_t cpu_ticks = SYST_CVR;
	uint32_t millis = systick_millis_count;
	uint32_t istatus = SCB_ICSR;  // bit 26 indicates if systick exception pending
	__enable_irq();

	if ((istatus & SCB_ICSR_PENDSTSET) && cpu_ticks > 50) millis++;
	cpu_ticks = ((F_CPU / 1000) - 1) - cpu_ticks;

    static_assert(F_CPU % sec == 0, "Please choose TimeUnit.usec to be a multiple of CPU cycles to keep timing precise");
	return millis * msec + cpu_ticks / (F_CPU / sec);
};

uint32_t Timestamp::get_value(TimeUnit tu) const {
    // To "extend" the period we assume that the timestamp is for close enough point in time to now.
    // Then, use millis and estimate the number of overflows happened.
    uint32_t cur_millis = millis();
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

