#pragma once

// Platform-specific constants

// Max stack size.
// NOTE: Stack overflow is not checked for now. We do check heap from growing into stack, though.
constexpr int stack_size = 4096;

// Returns max actually used stack.
int get_stack_high_water_mark();