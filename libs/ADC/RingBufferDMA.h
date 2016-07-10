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

#ifndef RINGBUFFERDMA_H
#define RINGBUFFERDMA_H

#include <Arduino.h> // for digitalWrite
#include "DMAChannel.h"
//#include <stdlib.h> // malloc


/** Class RingBufferDMA implements a circular buffer of fixed size (must be power of 2)
*   Code adapted from http://en.wikipedia.org/wiki/Circular_buffer#Mirroring
*/
class RingBufferDMA
{
    public:
        //! Constructor, buffer has a size len and stores the conversions of ADC number ADC_num
        RingBufferDMA(volatile int16_t* elems, uint32_t len, uint8_t ADC_num = 0);

        //! Destructor
        ~RingBufferDMA();

        //! Returns true if the buffer is full
        bool isFull();

        //! Returns true if the buffer is empty
        bool isEmpty();

        //! Read a value from the buffer, make sure it's not emtpy by calling isEmpty() first
        int16_t read();

        //! Start DMA operation
        void start();

        //! This function will be called when a DMA transfer finishes
        void void_isr();

        //! DMAChannel to handle all low level DMA code.
        DMAChannel* dmaChannel;

        //! Pointer to the elements of the buffer
        volatile int16_t* const p_elems;

        //! Size of buffer
        uint16_t b_size;

        //! ADC module of the instance
        uint8_t ADC_number;

        // the buffer needs to be aligned, so use malloc instead of new
        // see http://stackoverflow.com/questions/227897/solve-the-memory-alignment-in-c-interview-question-that-stumped-me/
        //uint8_t alignment;
        //void *p_mem;

        // this static pointer is set to point to this object and
        // call_dma_isr calls the void_isr()
        static RingBufferDMA *static_ringbuffer_dma;
        static void call_dma_isr(void);

    protected:
    private:

        //! Write a value into the buffer
        /** The actual value is copied by DMA, this function only updates the buffer pointers to reflect that fact.
        *
        */
        void write();

        //! Increases the pointer modulo 2*size-1
        uint16_t increase(uint16_t p);

        volatile uint32_t* const ADC_RA;

        //! Start pointer: Read here
        uint16_t b_start;
        //! End pointer: Write here
        uint16_t b_end;


};


#endif // RINGBUFFERDMA_H
