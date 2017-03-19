#pragma once

// Circular buffer/queue of elements of type T, with capacity C.
// NOTE: Both read and write indexes will freely overflow uint32_t and that's fine.
template<typename T, unsigned int C>
class CircularBuffer {
    static_assert(!(C & (C-1)), "Only power-of-two sizes of circular buffer are supported.");
    static_assert(C > 0, "Please provide positive capacity");
public:
    CircularBuffer() : read_idx_(0), write_idx_(0) {}

    inline bool empty() {
        return read_idx_ == write_idx_;
    }

    inline bool full() {
        return size() >= C;
    }

    inline unsigned long size() {
        return write_idx_ - read_idx_;
    }

    inline unsigned long max_size() {
        return C;
    }

    inline const T& front() const {
        return elems_[read_idx_ & (C-1)];
    }

    // Increment read counter.
    inline void pop_front() {
        if (!empty()) {
            read_idx_++;
        }
    }

    // Example usage:
    // T cur_elem;
    // while (c.dequeue(&cur_elem)) {
    //   // use cur_elem.
    // }
    inline bool dequeue(T *result_elem) {
        if (!empty()) {
            *result_elem = elems_[read_idx_ & (C-1)];
#ifdef __arm__
            // As this function can be used across irq context, make sure the order is correct.
            asm volatile ("dmb 0xF":::"memory");
#endif
            read_idx_++; 
            return true;
        } else {
            return false;
        }
    }

    // Example usage:
    // c.enqueue(elem);
    inline bool enqueue(const T& elem) {
        if (!full()) {
            elems_[write_idx_ & (C-1)] = elem;
#ifdef __arm__
            // As this function can be used across irq context, make sure the order is correct.
            asm volatile ("dmb 0xF":::"memory");
#endif
            write_idx_++;
            return true;
        } else {
            // TODO: We might want to increment an overflow counter here.
            return false;
        }
    }

private:
    volatile unsigned long read_idx_, write_idx_;
    T elems_[C];
};
