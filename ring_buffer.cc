/* Teensy 3.x, LC ADC library
 * https://github.com/pedvide/ADC
 * Copyright (c) 2015 Pedro Villanueva
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RingBuffer.h"



int RingBuffer::isFull() {
    return (end_ == (start_ ^ size_));
}

int RingBuffer::isEmpty() {
    return (end_ == start_);
}

void RingBuffer::write(int value) {
    elements[end_&(size_-1)] = value;
    if (IsFull()) { /* full, overwrite moves start pointer */
        start_ = increase(start_);
    }
    end_ = increase(end_);
}

int RingBuffer::read() {
    int result = elements[start_ & (size_ - 1)];
    start_ = increase(start_);
    return result;
}

// increases the pointer modulo 2*size-1
int RingBuffer::increase(int p) {
    return (p + 1) & (2 * size_ - 1);
}
