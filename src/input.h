#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/circular_buffer.h"
#include "common.h"

// We can used different Teensy modules to measure pulse timing, each with different pros and cons.
// Look into each input type's header for details.
enum InputType {
    kCMP = 0,  // Comparator
    kFTM = 1,  // Flexible Timer Module interrupts
    kPort = 2, // Digital input interrupt
    kMaxInputType
};


struct InputDefinition {
    uint32_t pin;  // Teensy PIN number
    bool pulse_polarity; // true = Positive, false = Negative.
    InputType input_type;
    uint32_t initial_cmp_threshold;
};


class InputNode 
    : public WorkerNode
    , public Producer<Pulse> {
public:
    // Setup the physical module, allocate pulse buffer and start getting interrupt signals.
    // This function needs to be overridden in children class.
    virtual void start() {
        pulses_buf_ = &static_pulse_bufs_[input_idx_];
    };

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

protected:
    // In all types of inputs, pulses come from irq handlers. We don't want to run any other code in 
    // irq context, so pulses go through pulses_buf_ circular buffer and produced in main "thread".
    virtual void do_work(Timestamp cur_time) {
        Pulse p;
        while (pulses_buf_->dequeue(&p))
            produce(p);
    }

    bool is_active_;
    uint32_t input_idx_;

    const static int pulses_buffer_len = 32;
    CircularBuffer<Pulse, pulses_buffer_len> *pulses_buf_;

private:
    static CircularBuffer<Pulse, pulses_buffer_len> static_pulse_bufs_[max_num_inputs];
};
 