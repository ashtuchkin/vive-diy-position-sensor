#pragma once
#include "messages.h"
#include "primitives/string_utils.h"

// Given pairs of pulse lens from 2 base stations, this class determines the phase for current cycle
// Phases are: 
//   0) Base 1 (B), horizontal sweep
//   1) Base 1 (B), vertical sweep
//   2) Base 2 (C), horizontal sweep
//   3) Base 2 (C), vertical sweep
// 
// TODO: We might want to introduce a more thorough check for a fix, using the average_error_ value.
class CyclePhaseClassifier {
public:
    CyclePhaseClassifier();

    // Process the pulse lengths for current cycle (given by incrementing cycle_idx).
    void process_pulse_lengths(uint32_t cycle_idx, const TimeDelta (&pulse_lens)[num_base_stations]);

    // Get current cycle phase. -1 if phase is not known (no fix achieved).
    int get_phase(uint32_t cycle_idx);
    
    // Reference to a pair of DataFrameBit-s
    typedef DataFrameBit (&DataFrameBitPair)[num_base_stations];

    // Get updated data bits from the pulse lens for current cycle.
    // Both bits are always returned, but caller needs to make sure they were updated this cycle by
    // checking DataFrameBit.cycle_idx == cycle_idx.
    DataFrameBitPair get_data_bits(uint32_t cycle_idx, const TimeDelta (&pulse_lens)[num_base_stations]);

    // Reset the state of this classifier - needs to be called if the cycle fix was lost.
    void reset();

    // Print debug information.
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    float expected_pulse_len(bool skip, bool data, bool axis);

    uint32_t prev_full_cycle_idx_;
    uint32_t phase_history_;

    int fix_level_;
    uint32_t phase_shift_;

    float pulse_base_len_;
    DataFrameBit bits_[num_base_stations];

    float average_error_;
    bool debug_print_state_;
};

