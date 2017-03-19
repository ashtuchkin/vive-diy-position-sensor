#include "debug_node.h"
#include "settings.h"

#include <exception>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>

#include <avr_functions.h>  // eeprom handling.
#include <avr_emulation.h>
#include <usb_serial.h>

#include <cxxabi.h>
using namespace abi;

// This file contains support functions that make it possible to use C++ exceptions and other niceties 
// in a meaningful way on Teensy.
// See https://andriidevel.blogspot.com/2016/05/size-cost-of-c-exception-handling-on.html for some tricks used.

// NOTE: gcc-arm-embedded uses 'newlib' as stdlib. To be properly used, newlib needs several "Syscall" functions
// to be implemented, see https://sourceware.org/newlib/libc.html#Syscalls .
// For Teensy, some of them are defined in teensy3/mk20dx128.c  but we need to 


// ====  1. Termination  ======================================================

// abort() is a C stdlib function to do an abnormal program termination.
// It's being called for example when exception handling mechanisms themselves fail. 
// It's not defined by default, so we need to implement it.
void abort() {
    // TODO: Write to eeprom.
    Serial.println("Aborted.");
    while(1);
}

#include <stdexcept>

// terminate_handler is called on std::terminate() and handles uncaught exceptions, exception-in-exceptions, etc.
// We install it statically below (__cxxabiv1::__terminate_handler) to avoid linking with default GCC 
// __verbose_terminate_handler, which adds 30kb of code.
// See also: https://gcc.gnu.org/onlinedocs/libstdc++/manual/termination.html
// https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/vterminate.cc#L44
// https://akrzemi1.wordpress.com/2011/10/05/using-stdterminate/
// 
[[noreturn]] void terminate_handler() {
    if (std::type_info *t = __cxa_current_exception_type()) {
        Serial.print("Uncaught exception: ");
        Serial.print(t->name());

        try { 
            throw; // Rethrow current exception to get its what() string.
        }
        catch(const std::exception& exc)
        {
            char const *w = exc.what();
            Serial.print("; what() = ");
            Serial.print(w);
        }
        catch(...) { }
    } else {
        Serial.print("Terminate handler");
    }
    Serial.println();

    //eeprom_write_block(t->name(), nullptr, 10);

    // Reboot
    while(1);
}

namespace __cxxabiv1 { 
    // Default terminate handler.
    std::terminate_handler __terminate_handler = terminate_handler;
}

// TODO: We might want to re-define _exit() and __cxa_pure_virtual() as well to provide meaningful actions.
// TODO: Define correct actions for hard failures like division by zero or invalid memory access (see -fnon-call-exceptions)

// ====  2. Memory & Stack  ===================================================
// Low-level _sbrk() syscall used to allocate memory is defined in teensy3/mk20dx128.c and very simple - just moves 
// __brkval, current top of the heap. We're rewriting it to add stack clashing check.
// malloc() uses _sbrk(). operator new() uses malloc().
// Teensy defines a basic "operator new()", but it's too simple for our case, so we use standard one instead.

// Nice lib to get memory stats: https://github.com/michaeljball/RamMonitor
// Or, use standard interface mallinfo(); (uordblks = bytes in use, fordblks = bytes free.)

// Stack starts at the end of ram and grows down, towards the heap.
// It's a bit hard to know how much stack is actually used. There are some static methods like the one in 
// http://www.dlbeer.co.nz/oss/avstack.html, some compile-time warnings help (-Wstack-usage=256), plus we can
// use dynamic methods like filling stack with a pattern.
// NOTE: Stack overflow is not actually checked, but we're guaranteed that heap don't grow into stack of given size.
// A proper solution would require setting up MMU correctly, which we don't want to do now.

// Max stack size.
// NOTE: Stack overflow is not checked for now. We do check heap from growing into stack, though.
constexpr int stack_size = 4096;

extern char *__brkval;  // top of heap (dynamic ram): grows up towards stack
extern char _estack;    // bottom of stack, top of ram: stack grows down towards heap

extern "C" void * _sbrk(int incr)
{
    // Check we're not overwriting the stack at the end of ram.
    if (__brkval + incr > &_estack - stack_size) {
        errno = ENOMEM;
        return (void *)-1;
    }
	char *prev = __brkval;
	__brkval += incr;
	return prev;
}

// Check the high water mark of stack by filling the memory with a pattern and then checking
// how much memory still has it.
struct StackFillChecker {
    const uint32_t pattern = 0xFAAFBABA;
    StackFillChecker() {
        char *frame_address = (char*)__builtin_frame_address(0);
        for (uint32_t *p = (uint32_t*)(&_estack - stack_size); p < (uint32_t*)(frame_address - 64); p++)
            *p = pattern;
    }

    // Returns max used stack in bytes.
    int get_high_water_mark() {
        char *frame_address = (char*)__builtin_frame_address(0);
        for (uint32_t *p = (uint32_t*)(&_estack - stack_size); p < (uint32_t*)frame_address; p++)
            if (*p != pattern)
                return &_estack - (char *)p;
        return &_estack - frame_address;
    }
};
static StackFillChecker static_stack_fill_checker;

int get_stack_high_water_mark() {
    return static_stack_fill_checker.get_high_water_mark();
}

// ====  3. Assertions  =======================================================

[[noreturn]] void __assert_func2(const char *function_name, const char *expression) {
    // TODO: Write to EEPROM.
    // OR throw exception.
    Serial.printf("assert(%s) in %s failed.\n", expression, function_name);

    //char buf[128];
    //snprintf(buf, sizeof(buf), "assert(%s) failed in %s", expression, function_name);
    //eeprom_write_block(buf, nullptr, sizeof(buf));
    // Reboot.
    while(1);
}

// assert() macro in debug mode is defined to call __assert_func when assertion fails. 
// We must redefine it statically to avoid linking to additional 15kb of code.
// Default implementation: https://github.com/bminor/newlib/blob/master/newlib/libc/stdlib/assert.c#L53
[[noreturn]] void __assert_func(const char *file, int line, const char *function_name, const char *expression) {
    __assert_func2(function_name, expression);
}


// ====  4. Writing to file descriptors =======================================

// In Print.cpp we define _write() syscall and then use vdprintf() to write. Print class pointer is used
// as file descriptor.


// ====  5. Stack overflow protection  ========================================
//  -fstack-usage -fdump-rtl-dfinish  gcc options can be used to get static stack structure.
// See http://stackoverflow.com/questions/6387614/how-to-determine-maximum-stack-usage-in-embedded-system-with-gcc
// Unfortunately, this probably doesn't work with virtual functions.
// 
// There's another way: write to unused stack some predefined values and check it periodically
// This won't catch stack overflow, but will tell approximate used stack size.
// 
// Yet another way is to place stack in the beginning of RAM. That way we'll fail hard.
// 

// ====  6. Yield  ============================================================
// This needs to be replaced with empty body to avoid linking to all the Serial-s and save memory.
void yield() {}


// ====  7. Printing debug information ========================================

// Link-time constant markers. Note, you need the *address* of these.
extern char _sdata;   // start of static data
extern char _edata;
extern char _sbss;
extern char _ebss;    // end of static data; bottom of heap
extern char _estack;  // bottom of stack, top of ram: stack grows down towards heap

void print_platform_memory_info(PrintStream &stream) {
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

// ====  8. Basic configuration helpers =======================================

void restart_system() {
    SCB_AIRCR = 0x5FA0004; // Restart Teensy.
}

void eeprom_read(uint32_t eeprom_addr, void *dest, uint32_t len) {
    eeprom_read_block(dest, (void *)eeprom_addr, len);
}

void eeprom_write(uint32_t eeprom_addr, const void *src, uint32_t len) {
    eeprom_write_block(src, (void *)eeprom_addr, len);
}
