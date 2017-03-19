#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/circular_buffer.h"
#include "primitives/static_registration.h"
#include "messages.h"

// We can use different Teensy hardware features to measure pulse timing, each with different pros and cons.
// Look into each input type's header for details.
enum class InputType {
    kCMP = 0,  // Comparator
    kTimer = 1,// Timer Module interrupts
    kPort = 2, // Digital input interrupt
};
constexpr int kInputTypeCount = 3;

// Stored definition of an InputNode.
struct InputDef {
    uint32_t pin;  // Teensy PIN number
    bool pulse_polarity; // true = Positive, false = Negative.
    InputType input_type;
    uint32_t initial_cmp_threshold;

    void print_def(uint32_t idx, PrintStream &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream);
};


// Base class for input nodes. They all produce Pulse-s.
class InputNode 
    : public WorkerNode
    , public Producer<Pulse> {
public:
    // Create input node of needed type from given configuration.
    static std::unique_ptr<InputNode> create(uint32_t input_idx, const InputDef &def);
    typedef StaticRegistrar<decltype(create)*> CreatorRegistrar;

    virtual void do_work(Timestamp cur_time);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

protected:
    InputNode(uint32_t input_idx);
    void enqueue_pulse(Timestamp start, TimeDelta len);

private:
    // Index of this input.
    uint32_t input_idx_;

    // We keep the pulse buffer to move Pulse-s from irq context to main thread context.
    static constexpr int pulses_buffer_len = 32;
    CircularBuffer<Pulse, pulses_buffer_len> pulses_buf_;
};
