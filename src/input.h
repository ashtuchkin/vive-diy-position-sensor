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

class Print;
class HashedWord;

struct InputDefinition {
    uint32_t pin;  // Teensy PIN number
    bool pulse_polarity; // true = Positive, false = Negative.
    InputType input_type;
    uint32_t initial_cmp_threshold;

    void print_def(uint32_t idx, Print &err_stream);
    bool parse_def(HashedWord *input_words, Print &err_stream);
};


constexpr int pulses_buffer_len = 32;
typedef CircularBuffer<Pulse, pulses_buffer_len> PulseCircularBuffer;

class InputNode 
    : public WorkerNode
    , public Producer<Pulse> {
public:
    // Create input node of needed type from given configuration.
    static InputNode *create(uint32_t input_idx, const InputDefinition &def);

    virtual void do_work(Timestamp cur_time);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

protected:
    InputNode(uint32_t input_idx);

    void enqueue_pulse(Timestamp start, TimeDelta len);

private:
    // Index of this input.
    uint32_t input_idx_;

    // We keep the pulse buffer to move Pulse-s from irq context to main thread context.
    PulseCircularBuffer pulses_buf_;
};
 