#pragma once
#include "primitives/workers.h"
#include <memory>
#include <Print.h>

class Stream;
class DetachablePrint;

// This node calls debug_cmd and debug_print for all pipeline nodes periodically,
// provides some other debug facilities and blinks LED.
class DebugNode : public WorkerNode {
public:
    DebugNode(Pipeline *pipeline, Stream &debug_stream);

    virtual void do_work(Timestamp cur_time);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print &stream);

    // Detachable print stream - used to stop output to usb serial when debug mode is on.
    Print &stream();

    virtual ~DebugNode() = default;
private:
    Pipeline *pipeline_;
    Stream &debug_stream_;
    std::unique_ptr<DetachablePrint> detachable_print_;
    Timestamp debug_print_period_, blinker_period_;
    uint32_t continuous_debug_print_;
    bool print_debug_memory_;
};

// Detachable Print class is used to stop outputting geometry data when debug mode is on.
class DetachablePrint : public Print {
public:
    DetachablePrint(Print &source) : source_(source), attached_(true) {}
    void set_attached(bool attached) { attached_ = attached; }
    bool is_attached() const { return attached_; }

    // Main printing methods.
	virtual size_t write(uint8_t b) {
        return attached_ ? source_.write(b) : 0;
    }
	virtual size_t write(const uint8_t *buffer, size_t size) {
        return attached_ ? source_.write(buffer, size) : 0;
    }

    virtual ~DetachablePrint() = default;
private:
    Print &source_;
    bool attached_;
};
