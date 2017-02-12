#pragma once

// Circular buffer/queue of elements of type T, with capacity C.
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

    // Example usage:
    // T cur_elem;
    // while (c.dequeue(&cur_elem)) {
    //   // use cur_elem.
    // }
    inline bool dequeue(T *result_elem) {
        if (!empty()) {
            *result_elem = elems_[read_idx_ & (C-1)];
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
            elems_[write_idx_ & (C-1)] = elem;
            asm volatile ("dmb 0xF":::"memory");
            write_idx_++; // NOTE: Index will freely overflow uint32_t and that's fine.
            return true;
        } else {
            // TODO: We might want to increment an overflow counter here.
            return false;
        }
    }

private:
    unsigned long read_idx_, write_idx_;
    T elems_[C];
};
