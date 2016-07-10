#ifndef _UTIL_PARITY_H_
#define _UTIL_PARITY_H_

static inline uint8_t parity_even_bit(uint8_t x) __attribute__((pure, always_inline, unused));
static inline uint8_t parity_even_bit(uint8_t x)
{
	x ^= x >> 1;
	x ^= x >> 2;
	x ^= x >> 4;
	return x & 1;
}

#endif
