#include "debug_node.h"
#include <assert.h>

#include "settings.h"
#include "led_state.h"
#include "print_helpers.h"


DebugNode::DebugNode(Pipeline *pipeline)
    : pipeline_(pipeline)
    , continuous_debug_print_(0)
    , stream_idx_(0x1000)
    , output_attached_(true)
    , print_debug_memory_(false) {
    assert(pipeline);
}

void DebugNode::consume_line(char *input_cmd, Timestamp time) {
    // Process debug input commands
    bool print_debug = !output_attached_ && !continuous_debug_print_;
    set_output_attached(false);
    continuous_debug_print_ = 0;

    HashedWord* hashed_words = hash_words(input_cmd);
    bool res = !*hashed_words || pipeline_->debug_cmd(hashed_words);
    if (!output_attached_ && !continuous_debug_print_) {
        DataChunkPrintStream printer(this, time, stream_idx_);
        if (!res)
            printer.printf("Unknown command.\n");                
        else if (print_debug)
            pipeline_->debug_print(printer);
        printer.printf("debug> ");
    }
}

void DebugNode::do_work(Timestamp cur_time) {
    // Print current debug state if continuous printing is enabled.
    if (continuous_debug_print_ > 0 
            && throttle_ms(TimeDelta(continuous_debug_print_, ms), cur_time, &continuous_print_period_)) {
        DataChunkPrintStream printer(this, cur_time, stream_idx_);
        pipeline_->debug_print(printer);
    }

    // Update led pattern.
    update_led_pattern(cur_time);
}

// Sometimes the same output is used both for debug and to print values. We want to detach the values stream
// while we're in debug mode.
void DebugNode::set_output_attached(bool attached) {
    if (output_attached_ == attached)
        return;

    // Send command to the output node we're working with.
    Producer<OutputCommand>::produce(attached 
        ? OutputCommand{.type = OutputCommandType::kMakeNonExclusive} 
        : OutputCommand{.type = OutputCommandType::kMakeExclusive, .stream_idx = stream_idx_});
    
    output_attached_ = attached;
}

bool DebugNode::debug_cmd(HashedWord *input_words) {
    switch (*input_words++) {
    case "debug"_hash:
        switch (*input_words++) {
        case "mem"_hash:
        case "memory"_hash:
            print_debug_memory_ = !print_debug_memory_;
            return true;
        }
        break;
    
    case "!"_hash: settings.restart_in_configuration_mode(); return true;
    case "o"_hash: set_output_attached(true); return true;
    case "c"_hash:
        uint32_t val;
        if (!*input_words) {
            continuous_debug_print_ = 1000; continuous_print_period_ = Timestamp::cur_time(); return true;
        } else if (input_words->as_uint32(&val) && val >= 10 && val <= 100000) {
            continuous_debug_print_ = val; continuous_print_period_ = Timestamp::cur_time(); return true;
        }
        break;
    }
    return false;
}

void DebugNode::debug_print(PrintStream &stream) {
    if (print_debug_memory_) {
        print_platform_memory_info(stream);
    }
}
