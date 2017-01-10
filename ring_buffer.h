// Templated fixed-size ring buffer for embedded; adapted from Teensy library.
// Author: mierle@gmail.com (Keir Mierle)
//
// Original copyright header from Teensy libraries:
//
// Teensy 3.x, LC ADC library
// https://github.com/pedvide/ADC
// Copyright (c) 2015 Pedro Villanueva
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

// A circular buffer of fixed size; must be a power of 2. Fixed size buffer is
// inlined into the object; no allocations are done. T must have a copy
// constructor defined.
//
// TODO(keir): Add a compile time assert that num_elements is a power of 2.
template<typename T, int num_elements>
class RingBuffer {
 public:
  bool IsFull() {
    return end_ == (start_ ^ num_elements);
  }
  bool IsEmpty() {
    return end_ == start_;
  }

  // Push a new element into the buffer by copying. If there is space available,
  // returns true to indicate that no values were clobbered. If the buffer is
  // full and an element was clobbered to place the passed one, returns false.
  bool PushFront(const T& value) {
    // Note mask to strip off leading bit, due to clever indexing scheme.
    elements_[end_ & (num_elements - 1)] = value;

    bool did_not_clobber = true;
    if (IsFull()) {
      // If the buffer is full, bump the start location up, since the oldest
      // element in the buffer was just overwritten with the new one.
      start_ = NextIndex(start_);
      did_not_clobber = false;
    }
    end_ = NextIndex(end_);
    return did_not_clobber;
  }

  // Pops and returns the last element. Behaviour is undefined on empty buffers.
  T PopBack() {
    int last_item_index = start_ & (num_elements - 1);
    start_ = NextIndex(start_);
    return elements_[last_item_index];
  }

  // If there are elements in the buffer, pops the last element, copying it
  // onto *value, and returns true. If the buffer is empty, returns false.
  bool PopBack(T* value) {
    if (IsEmpty()) {
      return false;
    }
    *value = elements_[start_ & (num_elements - 1)];
    start_ = NextIndex(start_);
    return true;
  }

 private:
  // Get the next index after the given one, wrapping.
  int NextIndex(int p) {
    return (p + 1) & (2 * num_elements - 1);
  }

  int start_ = 0;

  // End points to the next index an element will be written to, but may take
  // on values up to 2 * num_elements - 1 to avoid extra cycles. This is
  // stripped off in PushFront().
  int end_ = 0;

  T elements_[num_elements];
};

#endif // RING_BUFFER_H_
