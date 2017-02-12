#pragma once
#include "primitives/workers.h"

class Stream;

// This node calls debug_cmd and debug_print for all pipeline nodes periodically,
// provides some other debug facilities and blinks LED.
class DebugNode : public WorkerNode {
public:
    DebugNode(Pipeline *pipeline, Stream &debug_stream);

    virtual void do_work(Timestamp cur_time);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print &stream);

private:
    Pipeline *pipeline_;
    Stream &debug_stream_;
    Timestamp debug_print_period_, blinker_period_;
    bool print_debug_memory_;
};
