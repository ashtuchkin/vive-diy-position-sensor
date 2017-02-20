#include "debug_node.h"
#include <assert.h>
#include <malloc.h>

#include "settings.h"
#include "led_state.h"
#include "platform.h"
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
        DataChunkPrint printer(this, time, stream_idx_);
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
        DataChunkPrint printer(this, cur_time, stream_idx_);
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
        ? OutputCommand{.type = OutputCommand::kMakeNonExclusive} 
        : OutputCommand{.type = OutputCommand::kMakeExclusive, .stream_idx = stream_idx_});
    
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


// ======  System-wide debug metrics  =========================================

// Link-time constant markers. Note, you need the *address* of these.
extern char _sdata;   // start of static data
extern char _edata;
extern char _sbss;
extern char _ebss;    // end of static data; bottom of heap
extern char _estack;  // bottom of stack, top of ram: stack grows down towards heap

// Dynamic values from mk20dx128.c
extern char *__brkval; // top of heap (dynamic ram): grows up towards stack

void DebugNode::debug_print(Print &stream) {
    if (print_debug_memory_) {
        uint32_t static_data_size = &_ebss - (char*)((uint32_t)&_sdata & 0xFFFFF000);
        uint32_t allocated_heap = __brkval - &_ebss;
        char c, *top_stack = &c;
        int32_t unallocated = (&_estack - stack_size) - __brkval;
        int32_t stack_max_used = get_stack_high_water_mark();
        uint32_t stack_used = &_estack - top_stack;

        struct mallinfo m = mallinfo();
        stream.printf("RAM: static %d, heap %d (used %d, free %d), unalloc %d, stack %d (used %d, max %d)\n", 
            static_data_size, allocated_heap, m.uordblks, m.fordblks, unallocated, stack_size, stack_used, stack_max_used);
    }
}
