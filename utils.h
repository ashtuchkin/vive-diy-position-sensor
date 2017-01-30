#pragma once

// Simplified version of static asserts.
template <bool b> struct StaticAssert;
template <>       struct StaticAssert<true>{ enum { value = 1 }; };
#define _STATIC_JOIN_(x, y) x ## y
#define _STATIC_JOIN(x, y) _STATIC_JOIN_(x, y)
#define STATIC_ASSERT(B) enum { _STATIC_JOIN(static_assert_, __LINE__) = sizeof(StaticAssert<(bool)( B )>) }


// Circular buffer/queue of elements of type T, with max size N.
template<typename T, unsigned int N>
class CircularBuffer {
    STATIC_ASSERT(!(N & (N-1))); // Only power-of-two sizes of circular buffer are supported.
    STATIC_ASSERT(N > 0);
public:
    inline bool empty() {
        return read_idx_ == write_idx_;
    }

    inline bool full() {
        return size() >= N;
    }

    inline uint32_t size() {
        return write_idx_ - read_idx_;
    }

    inline uint32_t max_size() {
        return N;
    }

    // Example usage:
    // T cur_elem;
    // while (c.dequeue(&cur_elem)) {
    //   // use cur_elem.
    // }
    inline bool dequeue(T *result_elem) {
        if (!empty()) {
            *result_elem = elems_[read_idx_ & (N-1)];
            asm volatile ("dmb 0xF":::"memory");
            read_idx_++; // NOTE: Index will freely overflow uint32_t and that's fine.
            return true;
        } else {
            return false;
        }
    }

    // Example usage:
    // c.enqueue(elem);
    inline bool enqueue(const T& elem) {
        if (!full()) {
            elems_[write_idx_ & (N-1)] = elem;
            asm volatile ("dmb 0xF":::"memory");
            write_idx_++; // NOTE: Index will freely overflow uint32_t and that's fine.
            return true;
        } else {
            // TODO: We might want to increment an overflow counter here.
            return false;
        }
    }

private:
    uint32_t read_idx_, write_idx_;
    T elems_[N];
};


// Returns true only once per period_ms. cur_time is the current time in ms. prev_period is a pointer to uint32_t that
// keeps data between calls, should not be used otherwise; slips will contain the number of skipped periods, if provided.
// Usage:
// void loop() {
//     static uint32_t block_period = 0;
//     if (throttle_ms(1000, millis(), &block_period)) {
//         // This block will be executed once every 1000 ms.
//     }   
// }
inline bool throttle_ms(uint32_t period_ms, uint32_t cur_time, uint32_t *prev_period, uint32_t *slips = NULL) {
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

