// Simple version of vector with dynamic length, but fixed capacity.
// Only compatible with value or POD types, not classes (it doesn't do construction/destruction).
#pragma once
#include <type_traits>
#include <assert.h>

// Type T, Capacity C.
template<typename T, unsigned C> 
class Vector {
    static_assert(C > 0, "Please provide positive capacity");
    static_assert(std::is_trivially_destructible<T>(), "Vector only works on simple types");
    static_assert(std::is_trivially_copyable<T>(), "Vector only works on simple types");
public:
    Vector() : size_{} {}
    inline unsigned long size() const { return size_; }
    inline unsigned long max_size() const { return C; }
    inline bool empty() const { return size_ == 0; }
    inline bool full() const { return size_ >= C; }
    inline T &operator[](unsigned idx) { return elems_[idx]; }
    inline const T &operator[](unsigned idx) const { return elems_[idx]; }

    inline void set_size(unsigned long size) {
        assert(size <= C);
        size_ = size;
    }

    inline void push(const T &elem) {
        if (!full()) {
            elems_[size_++] = elem;
        } else {
            // assert.
        }
    }

    inline T pop() {
        if (!empty()) {
            return elems_[--size_];
        } else {
            // assert.
            return elems_[0];
        }
    }

    inline void clear() {
        size_ = 0;
    }

private:
    unsigned long size_;
    T elems_[C];
};
