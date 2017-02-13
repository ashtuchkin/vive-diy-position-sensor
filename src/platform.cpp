#include <exception>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

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


// ====  1. Termination  ================================

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
// TODO: Define correct actions for hard failures like division by zero or invalid memory access.

// ====  2. Memory ================================

// Low-leve _sbrk() syscall to allocate memory is defined in teensy3/mk20dx128.c and very simple - just moves 
// __brkval, current top of the heap.
// malloc() uses _sbrk().
// Teensy defines basic "operator new()", but it's too simple for our case, so we use standard one instead.

// Nice lib to get memory stats: https://github.com/michaeljball/RamMonitor
// Or, use standard interface mallinfo(); (uordblks = bytes in use, fordblks = bytes free.)


// ====  3. Assertions  ================================

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


// ====  4. Writing to file descriptors ================================

// In Print.cpp we define _write() syscall and then use vdprintf() to write. Print class pointer is used
// as file descriptor.


// ====  5. Stack overflow protection  ================================
//  -fstack-usage -fdump-rtl-dfinish  gcc options can be used to get static stack structure.
// See http://stackoverflow.com/questions/6387614/how-to-determine-maximum-stack-usage-in-embedded-system-with-gcc
// Unfortunately, this probably doesn't work with virtual functions.
// 
// There's another way: write to unused stack some predefined values and check it periodically
// This won't catch stack overflow, but will tell approximate used stack size.
// 
// Yet another way is to place stack in the beginning of RAM. That way we'll fail hard.
// 

// ====  6. Yield  =========================================
// This needs to be replaced with empty body to avoid linking to all the Serial-s and save memory.
void yield() {}
