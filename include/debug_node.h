#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/string_utils.h"
#include "print_helpers.h"

// This node calls debug_cmd and debug_print for all pipeline nodes periodically,
// provides some other debug facilities and blinks LED.
class DebugNode 
    : public WorkerNode
    , public DataChunkLineSplitter
    , public Producer<DataChunk>
    , public Producer<OutputCommand> {
public:
    DebugNode(Pipeline *pipeline);

    virtual void consume_line(char *line, Timestamp time);
    virtual void do_work(Timestamp cur_time);
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    void set_output_attached(bool attached);

    Pipeline *pipeline_;
    Timestamp continuous_print_period_;
    uint32_t continuous_debug_print_;
    uint32_t stream_idx_;
    bool output_attached_;
    bool print_debug_memory_;
};

// This function needs to be defined by the platform.
void print_platform_memory_info(PrintStream &stream);
