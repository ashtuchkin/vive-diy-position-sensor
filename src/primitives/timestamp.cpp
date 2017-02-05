#include "timestamp.h"
#include <Arduino.h>

Timestamp Timestamp::cur_time() { 
    return micros() * usec;  // TODO: Make it more precise.
};
