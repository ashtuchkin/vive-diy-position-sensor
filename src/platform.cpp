#include <exception>
#include <assert.h>
#include <stdlib.h>

// This file contains support functions that make it possible to use C++ exceptions and other niceties 
// in a meaningful way on Teensy.
// See https://andriidevel.blogspot.com/2016/05/size-cost-of-c-exception-handling-on.html for some tricks used.

// NOTE: gcc-arm-embedded uses 'newlib' as stdlib. To be properly used, newlib needs several "Syscall" functions
// to be implemented, see https://sourceware.org/newlib/libc.html#Syscalls .
// For Teensy, some of them are defined in teensy3/mk20dx128.c  but we need to 


// ====  1. Termination  ================================

// abort() is a C stdlib function to do an abnormal program termination.
// It's being called for example when exception handling mechanisms themselves fail. 
// It's not defined by default, so we need to implement it.
void abort() {
    // TODO: Write to eeprom.
    while(1);
}

// terminate_handler is called on std::terminate() and handles uncaught exceptions, exception-in-exceptions, etc.
// We install it statically below (__cxxabiv1::__terminate_handler) to avoid linking with default GCC 
// __verbose_terminate_handler, which adds 30kb of code.
// See also: https://gcc.gnu.org/onlinedocs/libstdc++/manual/termination.html
// https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/vterminate.cc#L44
void terminate_handler() {
    // TODO: Get exception name and write to eeprom.
    while(1);
}

namespace __cxxabiv1 { 
    std::terminate_handler __terminate_handler = terminate_handler;  // Default terminate handler for gcc c++ impl.
}

// TODO: We might want to re-define _exit() and __cxa_pure_virtual() as well to provide meaningful actions.


// ====  2. Memory ================================

// Low-leve _sbrk() syscall to allocate memory is defined in teensy3/mk20dx128.c and very simple - just moves 
// __brkval, current top of the heap.
// malloc() uses _sbrk().
// Teensy defines basic "operator new()", but it's too simple for our case, so we use standard one instead.

// Nice lib to get memory stats: https://github.com/michaeljball/RamMonitor
// Or, use standard interface mallinfo(); (uordblks = bytes in use, fordblks = bytes free.)


// ====  3. Assertions  ================================

// assert() macro in debug mode is defined to call __assert_func when assertion fails. 
// We must redefine it statically to avoid linking to additional 15kb of code.
// Default implementation: https://github.com/bminor/newlib/blob/master/newlib/libc/stdlib/assert.c#L53
void __assert_func(const char *file, int line, const char *function_name, const char *expression) {
    // TODO: Write to EEPROM.
    while(1);
}


