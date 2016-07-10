#ifndef WiringPrivate_h
#define WiringPrivate_h

#include <stdio.h>
#include <stdarg.h>

#include "wiring.h"

#ifndef cbi
#define cbi(sfr, bit) ((sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) ((sfr) |= _BV(bit))
#endif

typedef void (*voidFuncPtr)(void);

#endif


