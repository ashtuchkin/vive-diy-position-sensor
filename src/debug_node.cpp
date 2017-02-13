#include "debug_node.h"
#include <assert.h>
#include <malloc.h>

#include "settings.h"
#include "led_state.h"

#include <Stream.h>
#include <core_pins.h>
#include <pins_arduino.h>  // For blinker


DebugNode::DebugNode(Pipeline *pipeline, Stream &debug_stream) 
    : pipeline_(pipeline)
    , debug_stream_(debug_stream)
    , print_debug_memory_(false) {
    assert(pipeline);
}

void DebugNode::do_work(Timestamp cur_time) {
    // Process debug input commands
    while (char *input_cmd = read_line(debug_stream_)) {
        HashedWord* hashed_words = hash_words(input_cmd);
        if (*hashed_words && !pipeline_->debug_cmd(hashed_words)) 
            debug_stream_.println("Unknown command.");
    }

    // Print current debug state
    if (throttle_ms(TimeDelta(1000, ms), cur_time, &debug_print_period_))
        pipeline_->debug_print(debug_stream_);
    
    // Update led pattern.
    update_led_pattern(cur_time);
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
    
    case "!"_hash:
        settings.restart_in_configuration_mode();
        return true;
    }
    return false;
}

// Link-time constant markers. Note, you need the *address* of these.
extern char _sdata;   // start of static data
extern char _edata;
extern char _sbss;
extern char _ebss;    // end of static data; bottom of heap
extern char _estack;  // bottom of stack, top of ram: stack grows down towards heap

// Top of heap from mk20dx128.c
extern char *__brkval; // top of heap (dynamic ram): grows up towards stack

void DebugNode::debug_print(Print &stream) {
    if (print_debug_memory_) {
        uint32_t static_data_size = &_ebss - (char*)((uint32_t)&_sdata & 0xFFFFF000);
        uint32_t allocated_heap = __brkval - &_ebss;
        char c, *top_stack = &c;
        int32_t heap_to_stack_distance = top_stack - __brkval;
        uint32_t stack_size = &_estack - top_stack;
        struct mallinfo m = mallinfo();
        stream.printf("RAM: static %d, heap %d (used %d, free %d), unalloc %d, stack %d\n", 
            static_data_size, allocated_heap, m.uordblks, m.fordblks, heap_to_stack_distance, stack_size);
    }
}
