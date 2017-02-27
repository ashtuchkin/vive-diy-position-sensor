#pragma once
#if __arm__  

// On ARM, half-precision floats are supported natively as __fp16 type.
typedef __fp16 fp16;

#else
#include <stdint.h>

// Simple emulation of half-precision floats. 
struct fp16 {
    uint16_t raw_value;

    __attribute__ ((noinline)) 
    explicit operator float() const {
        // Convert to float. While technically not valid due to strict aliasing rules, it is guaranteed to be supported by GCC.
        // https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Type%2Dpunning
        union {
            uint32_t ui;
            float f;
        } u;
        u.ui = h2f_internal();
        return u.f;
    }

    uint32_t h2f_internal() const {
        // Based on gnu arm implementation https://github.com/gcc-mirror/gcc/blob/master/libgcc/config/arm/fp16.c#L169
        uint32_t sign = (uint32_t)(raw_value & 0x8000) << 16;
        int32_t aexp = (raw_value >> 10) & 0x1f;
        uint32_t mantissa = raw_value & 0x3ff;

        if (aexp == 0x1f)
            return sign | 0x7f800000 | (mantissa << 13);

        if (aexp == 0) {
            if (mantissa == 0)
                return sign;

            int shift = __builtin_clz(mantissa) - 21;
            mantissa <<= shift;
            aexp = -shift;
        }

        return sign | (((aexp + 0x70) << 23) + (mantissa << 13));
    }
};

#endif
