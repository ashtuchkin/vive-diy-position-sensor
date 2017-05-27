// Classes to make dealing with wrappable timestamps easier.
// For a problem statement see e.g. http://arduino.stackexchange.com/a/12588
// Also, a utility function throttle_ms to run something periodically.
#pragma once
#include <stdint.h>

enum TimeUnit {
    usec = 3,            // This can be increased to get better time resolution.
    msec = usec * 1000,
    ms = msec,
    sec = msec * 1000,
};

// TimeDelta is conceptually a time period length. Can be negative, but shouln't be so large to overflow the int32.
class TimeDelta {
public:
    // Constructors
    constexpr TimeDelta(): time_delta_(0) {}  // Default constructor to make it possible to include it in structs.
    constexpr TimeDelta(int val, TimeUnit tu): time_delta_(val * tu) {}  // Main way to create TimeDelta: value and TimeUnit.
    constexpr TimeDelta(const TimeDelta& other) = default;
    constexpr TimeDelta& operator=(const TimeDelta& other) = default;

    constexpr int get_value(TimeUnit tu) const { return time_delta_ / tu; }

    // Simple comparison operators. Note: we only allow comparison between TimeDeltas.
    constexpr bool operator<(const TimeDelta& delta) const { return time_delta_ < delta.time_delta_; }
    constexpr bool operator>(const TimeDelta& delta) const { return time_delta_ > delta.time_delta_; }
    constexpr bool operator<=(const TimeDelta& delta) const { return time_delta_ <= delta.time_delta_; }
    constexpr bool operator>=(const TimeDelta& delta) const { return time_delta_ >= delta.time_delta_; }
    constexpr bool operator==(const TimeDelta& delta) const { return time_delta_ == delta.time_delta_; }
    constexpr bool operator!=(const TimeDelta& delta) const { return time_delta_ != delta.time_delta_; }

    // Simple arithmetic operators.
    constexpr TimeDelta operator+(const TimeDelta& delta) const { return time_delta_ + delta.time_delta_; }
    constexpr TimeDelta operator-(const TimeDelta& delta) const { return time_delta_ - delta.time_delta_; }
    constexpr TimeDelta operator*(int val) const { return time_delta_ * val; }
    constexpr TimeDelta operator/(int val) const { return time_delta_ / val; }
    constexpr float operator/(const TimeDelta& delta) const {return float(time_delta_) / float(delta.time_delta_); }

    // Arithmetic assignment operators.
    inline TimeDelta &operator+=(const TimeDelta& delta) { time_delta_ += delta.time_delta_; return *this; }
    inline TimeDelta &operator-=(const TimeDelta& delta) { time_delta_ -= delta.time_delta_; return *this; }
    inline TimeDelta &operator*=(int val) { time_delta_ *= val; return *this; }
    inline TimeDelta &operator/=(int val) { time_delta_ /= val; return *this; }
    
    // Helper functions
    inline bool within_range_of(const TimeDelta& other, const TimeDelta& half_range) {
        auto delta = time_delta_ - other.time_delta_;
        return -half_range.time_delta_ <= delta && delta <= half_range.time_delta_;
    }

private:
    constexpr TimeDelta(int delta): time_delta_(delta) {}
    int time_delta_;
    friend class Timestamp;
};

// Timestamp is conceptually a point in time with a given resolution, mod 2^32. 
// The class handles wrapping over uint32_t to add safety.
class Timestamp {
public:
    // Constructors.
    constexpr Timestamp(): time_(0) {} // Default constructor. We need it to be able to keep Timestamp in structs.
    constexpr Timestamp(const Timestamp& other) = default;
    constexpr Timestamp& operator=(const Timestamp& other) = default;

    // Get adjusted value of this timestamp in provided time unit. 
    // We try to "extend" the value outside of regular period of timestamp using current time in millis.
    // TODO: Add 64bit version of get_value.
    uint32_t get_value(TimeUnit tu) const;

    // Get raw value in 'ticks'.
    uint32_t get_raw_value() { return time_; }

    // Static getters.
    static Timestamp cur_time(); // Implementation will try to get the best resolution possible.

    // Create TimeDelta from a pair of Timestamps. Note that the wrapping is handled here correctly as we're converting to a signed int.
    constexpr TimeDelta operator-(const Timestamp& other) const { return time_ - other.time_; }

    // Simple arithmetic operators with TimeDelta-s. Both operands are converted to uint32_t
    constexpr Timestamp operator+(const TimeDelta& delta) const { return time_ + delta.time_delta_; }
    constexpr Timestamp operator-(const TimeDelta& delta) const { return time_ - delta.time_delta_; }
    inline Timestamp &operator+=(const TimeDelta& delta) { time_ += delta.time_delta_; return *this; }
    inline Timestamp &operator-=(const TimeDelta& delta) { time_ -= delta.time_delta_; return *this; }

    // Simple wrapping-aware comparison operators using conversion to signed int.
    constexpr bool operator< (const Timestamp& other) const { return int(time_ - other.time_) <  0; }
    constexpr bool operator> (const Timestamp& other) const { return int(time_ - other.time_) >  0; }
    constexpr bool operator<=(const Timestamp& other) const { return int(time_ - other.time_) <= 0; }
    constexpr bool operator>=(const Timestamp& other) const { return int(time_ - other.time_) >= 0; }
    constexpr bool operator==(const Timestamp& other) const { return int(time_ - other.time_) == 0; }
    constexpr bool operator!=(const Timestamp& other) const { return int(time_ - other.time_) != 0; }

private:
    constexpr Timestamp(uint32_t time): time_(time) {}
    static uint32_t cur_time_millis(); // Helper function for get_value().
    uint32_t time_;  // Think about this as some global time mod 2^32.
};

// Returns true only once per period_time. cur_time is the current timestamp. block_prev_run is a pointer to Timestamp that
// keeps data between calls, should not be used otherwise; slips will contain the number of skipped periods, if provided.
// Usage:
// void loop() {
//     Timestamp cur_time = Timestamp::cur_time();       
//     static Timestamp block_prev_run(cur_time);
//     if (throttle_ms(TimeDelta(1000, msec), cur_time, &block_prev_run)) {
//         // This block will be executed once every 1000 ms.
//     }   
// }
inline bool throttle_ms(TimeDelta period_time, Timestamp cur_time, Timestamp *prev_run_time, unsigned *slips = 0) {
    unsigned cnt = 0;
    while (cur_time >= *prev_run_time + period_time) {
        *prev_run_time += period_time;
        cnt++;
    }
    if (slips && cnt > 1)
        (*slips) += (cnt-1);
    return cnt;
}
