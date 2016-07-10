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

#include "RingBufferDMA.h"

// Point static_ringbuffer_dma to an RingBufferDMA object
// so that object's isr is called when DMA finishes
RingBufferDMA *RingBufferDMA::static_ringbuffer_dma;
void RingBufferDMA::call_dma_isr(void) {
        static_ringbuffer_dma->void_isr();
}

RingBufferDMA::RingBufferDMA(volatile int16_t* elems, uint32_t len, uint8_t ADC_num) :
        p_elems(elems)
        , b_size(len)
        , ADC_number(ADC_num)
        , ADC_RA(&ADC0_RA + (uint32_t)0x20000*ADC_number)
        {

    b_start = 0;
    b_end = 0;

    // point to the correct ADC
    //ADC_RA = &ADC0_RA + (uint32_t)0x20000*ADC_number;

    dmaChannel = new DMAChannel(); // reserve a DMA channel

    //p_elems = new int16_t[len];


    // len must be power of two. extract the alignment
    //alignment = 32 - __builtin_clz(len);
    //uintptr_t mask = ~(uintptr_t)(len - 1);
    //p_mem = malloc(len+len-1);
    //p_elems = (int16_t *)(((uintptr_t)p_mem+len-1) & mask);

    // calculate mask
    //uint8_t mask = (1 << (alignment)) - 1;

    // To align to 16 bytes, allocate 15 bytes more and "move" the pointer to a 16 byte alignment, same for other sizes
    // we need to free(p_mem), the original pointer returned by malloc.
    //p_mem = malloc(len+len-1);
    //p_elems = (int16_t*)(((uintptr_t)p_mem+len-1) & ~ (uintptr_t)mask);


    //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
}

void RingBufferDMA::start() {

    // set up a DMA channel to store the ADC data
    // The idea is to have ADC_RA as a source,
    // the buffer as a circular buffer
    // each ADC conversion triggers a DMA transfer (transferCount(1)), of size 2 bytes (transferSize(2))

    dmaChannel->source(*ADC_RA);
//      TCD->SADDR = ADC_RA;
//		TCD->SOFF = 0;
//		TCD->ATTR_SRC = 2; // upper 8 bits of TCD->ATTR are TCD->ATTR_SRC
//		TCD->NBYTES = 4;
//		TCD->SLAST = 0;

    dmaChannel->destinationCircular((uint16_t*)p_elems, b_size); // 2*b_size is necessary for some reason
//    TCD->DADDR = p;
//    TCD->DOFF = 2;
//    TCD->ATTR_DST = ((31 - __builtin_clz(len)) << 3) | 1;
//    TCD->NBYTES = 2;
//    TCD->DLASTSGA = 0;
//    TCD->BITER = len / 2;
//    TCD->CITER = len / 2;

    dmaChannel->transferSize(2); // both SRC and DST size
//      TCD->NBYTES = 2;
//	    TCD->ATTR = (TCD->ATTR & 0xF8F8) | 0x0101;

    dmaChannel->transferCount(1); // transfer 1 value (2 bytes)
//    TCD->BITER = 1;
//    TCD->CITER = 1;

    dmaChannel->interruptAtCompletion();
//    TCD->CSR |= DMA_TCD_CSR_INTMAJOR;


	uint8_t DMAMUX_SOURCE_ADC = DMAMUX_SOURCE_ADC0;
    if(ADC_number==1){
        DMAMUX_SOURCE_ADC = DMAMUX_SOURCE_ADC1;
    }

    // point here so call_dma_isr actually calls this object's isr function
    static_ringbuffer_dma = this;

	dmaChannel->triggerAtHardwareEvent(DMAMUX_SOURCE_ADC); // start DMA channel when ADC finishes a conversion
	dmaChannel->enable();
	dmaChannel->attachInterrupt(call_dma_isr);

    //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
}


RingBufferDMA::~RingBufferDMA() {

    dmaChannel->detachInterrupt();
    dmaChannel->disable();
    delete dmaChannel;
}

void RingBufferDMA::void_isr() {
    //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
    write();
    dmaChannel->clearInterrupt();
}


bool RingBufferDMA::isFull() {
    return (b_end == (b_start ^ b_size));
}

bool RingBufferDMA::isEmpty() {
    return (b_end == b_start);
}

// update internal pointers
// this gets called only by the isr
void RingBufferDMA::write() {
    // using DMA:
    // call this inside the dma_isr to update the b_start and/or b_end pointers
    if (isFull()) { /* full, overwrite moves start pointer */
        b_start = increase(b_start);
    }
    b_end = increase(b_end);
}

int16_t RingBufferDMA::read() {

    if(isEmpty()) {
        return 0;
    }

    // using DMA:
    // read last value and update b_start
    int result = p_elems[b_start&(b_size-1)];
    b_start = increase(b_start);
    return result;
}

// increases the pointer modulo 2*size-1
uint16_t RingBufferDMA::increase(uint16_t p) {
    return (p + 1)&(2*b_size-1);
}
