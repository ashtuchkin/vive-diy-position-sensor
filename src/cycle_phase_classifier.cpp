#include "cycle_phase_classifier.h"

enum PhaseFixLevels {  // Unscoped enum because we use it more like set of constants.
    kPhaseFixNone = 0,
    kPhaseFixCandidate = 1,
    kPhaseFixAcquired = 4,
    kPhaseFixFinal = 16,
};

CyclePhaseClassifier::CyclePhaseClassifier()
    : prev_full_cycle_idx_()
    , phase_history_()
    , fix_level_(kPhaseFixNone)
    , phase_shift_()
    , pulse_base_len_()
    , bits_{}
    , average_error_()
    , debug_print_state_(false) {
    reset();
}


void CyclePhaseClassifier::process_pulse_lengths(uint32_t cycle_idx, const TimeDelta (&pulse_lens)[num_base_stations]) {
    int cur_phase_id = -1;
    if (pulse_lens[0] > TimeDelta(0, usec) && pulse_lens[1] > TimeDelta(0, usec)) {
        int cur_more = pulse_lens[0] > pulse_lens[1];
        if (cycle_idx == prev_full_cycle_idx_ + 1) {
            // To get current phase, we use simple fact that in phases 0 and 1, first pulse is shorter than the second,
            // and in phases 2, 3 it is longer. This allows us to estimate current phase using comparison between 
            // the pair of pulses in current cycle (cur_more) and the previous one.
            phase_history_ = (phase_history_ << 1) | cur_more;  // phase_history_ keeps a bit for each pulse comparison.
            static const char phases[4] = {1, 2, 0, 3};
            cur_phase_id = phases[phase_history_ & 0x3];  // 2 least significant bits give us enough info to get phase.
        } else {
            phase_history_ = cur_more;
        }
        prev_full_cycle_idx_ = cycle_idx;
    }

    // If we haven't achieved final fix yet, check the cur_phase_id is as expected.
    if (cur_phase_id >= 0 && fix_level_ < kPhaseFixFinal) {
        if (fix_level_ == kPhaseFixNone) {
            // Use current phase_id as the candidate.
            fix_level_ = kPhaseFixCandidate;
            phase_shift_ = (cur_phase_id - cycle_idx) & 0x3;

        } else {
            // Either add or remove confidence that the phase_shift_ is correct.
            int expected_phase_id = (cycle_idx + phase_shift_) & 0x3;
            fix_level_ += (cur_phase_id == expected_phase_id) ? +1 : -1;
        }
    }
}

inline float CyclePhaseClassifier::expected_pulse_len(bool skip, bool data, bool axis) {
    // See https://github.com/nairol/LighthouseRedox/blob/master/docs/Light%20Emissions.md
    return pulse_base_len_ + (skip << 2 | data << 1 | axis) * 10.416f;
}

CyclePhaseClassifier::DataFrameBitPair CyclePhaseClassifier::get_data_bits(uint32_t cycle_idx, const TimeDelta (&pulse_lens)[num_base_stations]) {
    // This is almost naive algorithm that tracks/adjusts just one variable, pulse_base_len_, with the assumption that
    // all pulses can be shorter/longer than ideal by the same amount.
    // We might need to introduce tracking of each phase mid_len if sensors are not linear enough.

    int phase_id = get_phase(cycle_idx);
    if (phase_id >= 0) {
        for (int b = 0; b < num_base_stations; b++) 
            if (pulse_lens[b] > TimeDelta(0, usec)) {
                bool skip = (phase_id >> 1) != b;
                bool axis = phase_id & 0x1;

                // Get the middle value of pulse width between bits 0 and 1.
                float mid_len = (expected_pulse_len(skip, true, axis) 
                                +expected_pulse_len(skip, false, axis)) * 0.5f;

                // Get the bit by comparison with mid value.
                int pulse_len = pulse_lens[b].get_value(usec);
                bool bit = pulse_len > mid_len;

                // Slowly adjust pulse_base_len_
                float error = pulse_len - expected_pulse_len(skip, bit, axis);
                pulse_base_len_ += error * 0.1f;
                float abs_error = error > 0 ? error : -error;
                average_error_ = average_error_ * 0.9f + abs_error * 0.1f;

                // Output bits if error is reasonable.
                if (abs_error < 5.0) {
                    bits_[b].bit = bit;
                    bits_[b].cycle_idx = cycle_idx;
                }
            }
    }
    return bits_;
}

int CyclePhaseClassifier::get_phase(uint32_t cycle_idx) {
    if (fix_level_ >= kPhaseFixAcquired) {
        return (cycle_idx + phase_shift_) & 0x3;
    } else {
        return -1;
    }
}

void CyclePhaseClassifier::reset() {
    fix_level_ = kPhaseFixNone;
    prev_full_cycle_idx_ = -1;
    pulse_base_len_ = 62.5f;
    for (int b = 0; b < num_base_stations; b++) {
        bits_[b].base_station_idx = b;
        bits_[b].cycle_idx = 0;
    }
}

bool CyclePhaseClassifier::debug_cmd(HashedWord *input_words) {
    if (*input_words++ == "phase"_hash) 
        switch (*input_words++) {
            case "show"_hash: debug_print_state_ = true; return true;
            case "off"_hash: debug_print_state_ = false; return true;
        }
    return false;
}
void CyclePhaseClassifier::debug_print(PrintStream &stream) {
    if (debug_print_state_) {
        stream.printf("CyclePhaseClassifier: fix %d, phase %d, pulse_base_len %f, history 0x%x, avg error %.1f us\n", 
            fix_level_, phase_shift_, pulse_base_len_, phase_history_, average_error_);
    }
}