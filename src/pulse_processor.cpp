#include "pulse_processor.h"
#include "message_logging.h"
#include <math.h>

// Pulse classification parameters.
constexpr TimeDelta min_short_pulse_len(2, usec);
constexpr TimeDelta min_long_pulse_len(40, usec);
constexpr TimeDelta max_long_pulse_len(300, usec);

constexpr TimeDelta long_pulse_starts_accepted_range(30, usec);
constexpr TimeDelta long_pulse_starts[num_base_stations] = {TimeDelta(0, usec), TimeDelta(410, usec)};

constexpr TimeDelta cycle_period(8333, usec);  // Total len of 1 cycle.
constexpr TimeDelta angle_center_len(4000, usec);
constexpr TimeDelta short_pulse_min_time = angle_center_len - cycle_period / 3;
constexpr TimeDelta short_pulse_max_time = angle_center_len + cycle_period / 3;
constexpr TimeDelta cycle_processing_point = short_pulse_max_time + TimeDelta(100, usec); // time from start of the cycle.

enum CycleFixLevels {  // Unscoped enum because we use it more like set of constants.
    kCycleFixNone = 0,
    kCycleFixCandidate = 1,  // From here we have a valid cycle_start_time_
    kCycleFixAcquired = 5,
    kCycleFixMax = 10,
};

PulseProcessor::PulseProcessor(uint32_t num_inputs) 
    : num_inputs_(num_inputs)
    , cycle_fix_level_(0)
    , cycle_idx_(0)
    , cycle_long_pulses_{}
    , cycle_short_pulses_{}
    , unclassified_long_pulses_{}
    , phase_classifier_{}
    , angles_frame_{}
    , time_from_last_long_pulse_(0, usec)
    , debug_print_state_(false) {
    angles_frame_.sensors.set_size(num_inputs);
    angles_frame_.phase_id = -1;
}

void PulseProcessor::consume(const Pulse& p) {
    if (p.pulse_len >= max_long_pulse_len) {
        // Ignore very long pulses.
    } else if (p.pulse_len >= min_long_pulse_len) { // Long pulse - likely sync pulse
        process_long_pulse(p);
    } else { // Short pulse - likely laser sweep
        process_short_pulse(p);
    }
}

void PulseProcessor::process_long_pulse(const Pulse &p) {
    if (cycle_fix_level_ == kCycleFixNone) {
        // Bootstrap mode. We keep the previous long pulse in unclassified_long_pulses_ vector.
        // With this algorithm 2 base stations needed for a fix. We search for a situation where the last pulse was 
        // second in last cycle, which means (8333-400) us difference in start time.
        if (unclassified_long_pulses_.size() > 0) {
            Pulse last_long_pulse = unclassified_long_pulses_.pop();
            unclassified_long_pulses_.clear();

            time_from_last_long_pulse_ = p.start_time - last_long_pulse.start_time;
            if (time_from_last_long_pulse_.within_range_of(cycle_period - long_pulse_starts[1], long_pulse_starts_accepted_range)) {
                // Found candidate first pulse.
                reset_cycle_pulses();
                cycle_fix_level_ = kCycleFixCandidate;
                cycle_start_time_ = p.start_time;
                cycle_idx_ = 0;
                phase_classifier_.reset();
            }
        }
    }

    // Put the pulse into either one of two buckets, or keep it as unclassified.
    bool pulse_classified = false;
    if (cycle_fix_level_ >= kCycleFixCandidate) {
        // Put pulse into one of two buckets by start time.
        TimeDelta time_from_cycle_start = p.start_time - cycle_start_time_;
        for (int i = 0; i < num_base_stations; i++) {
            if (time_from_cycle_start.within_range_of(long_pulse_starts[i], long_pulse_starts_accepted_range)) {  
                cycle_long_pulses_[i].push(p);
                pulse_classified = true;
                break;
            }
        }
    }
    if (!pulse_classified)
        unclassified_long_pulses_.push(p);
}

void PulseProcessor::process_short_pulse(const Pulse &p) {
    if (cycle_fix_level_ >= kCycleFixCandidate && p.input_idx < num_inputs_) {
        // TODO: Filter out pulses outside of current cycle.
        cycle_short_pulses_.push(p);
    }
}

void PulseProcessor::process_cycle_fix(Timestamp cur_time) {
    TimeDelta pulse_start_corrections[num_base_stations] = {}, pulse_lens[num_base_stations] = {};

    // Check if we have long pulses from at least one base station.
    if (cycle_long_pulses_[0].size() > 0 || cycle_long_pulses_[1].size() > 0) {
        // Increase fix level if we have pulses from both stations.
        if (cycle_fix_level_ < kCycleFixMax && cycle_long_pulses_[0].size() > 0 && cycle_long_pulses_[1].size() > 0) 
            cycle_fix_level_++;
        
        // Average out long pulse lengths and start times for each base station across sensors.
        // pulse_start_corrections is the delta between actual start time and expected start time.
        // TODO: Take into account previous cycles as well, i.e. adjust slowly.
        for (int b = 0; b < num_base_stations; b++)
            if (uint32_t num_pulses = cycle_long_pulses_[b].size()) {
                Timestamp expected_start_time = cycle_start_time_ + long_pulse_starts[b];
                for (uint32_t i = 0; i < num_pulses; i++) {
                    const Pulse &pulse = cycle_long_pulses_[b][i];
                    pulse_start_corrections[b] += pulse.start_time - expected_start_time;
                    pulse_lens[b] += pulse.pulse_len;
                }
                if (num_pulses > 1) {
                    pulse_start_corrections[b] /= num_pulses;
                    pulse_lens[b] /= num_pulses;
                }
            }

        // Send pulse lengths to phase classifier.
        phase_classifier_.process_pulse_lengths(cycle_idx_, pulse_lens);

        // If needed, get the data bits from pulse lengths and send them down the pipeline
        if (Producer<DataFrameBit>::has_consumers()) {
            CyclePhaseClassifier::DataFrameBitPair bits = phase_classifier_.get_data_bits(cycle_idx_, pulse_lens);
            for (int b = 0; b < num_base_stations; b++)
                if (bits[b].cycle_idx == cycle_idx_) {
                    bits[b].time = cycle_start_time_;
                    Producer<DataFrameBit>::produce(bits[b]);
                }
        }

    } else {
        // No long pulses this cycle. We can survive several of such cycles, but our confidence in timing sinks.
        cycle_fix_level_--;
    }

    // Given the cycle phase, we can put the angle timings to a correct bucket.
    int cycle_phase = phase_classifier_.get_phase(cycle_idx_);
    if (cycle_phase >= 0) {
        // From (potentially several) short pulses for the same input, we choose the longest one.
        Pulse *short_pulses[max_num_inputs] = {};
        TimeDelta short_pulse_timings[max_num_inputs] = {};
        uint32_t emitting_base = cycle_phase >> 1;
        Timestamp base_pulse_start = cycle_start_time_ + long_pulse_starts[emitting_base] + pulse_start_corrections[emitting_base];
        for (uint32_t i = 0; i < cycle_short_pulses_.size(); i++) {
            Pulse *p = &cycle_short_pulses_[i];
            uint32_t input_idx = p->input_idx;

            // To get better precision, we calculate pulse timing based on the long pulse from the same base station.
            TimeDelta pulse_timing = p->start_time + p->pulse_len / 2 - base_pulse_start;

            // Get longest laser pulse.
            if (short_pulse_min_time < pulse_timing && pulse_timing < short_pulse_max_time)
                if (!short_pulses[input_idx] || short_pulses[input_idx]->pulse_len < p->pulse_len) {
                    short_pulses[input_idx] = p;
                    short_pulse_timings[input_idx] = pulse_timing;
                }
        }

        // Calculate the angles for inputs where we saw short pulses.
        for (uint32_t i = 0; i < num_inputs_; i++) 
            if (short_pulses[i]) {
                SensorAngles &angles = angles_frame_.sensors[i];
                angles.angles[cycle_phase] = (short_pulse_timings[i] - angle_center_len) / cycle_period * (float)M_PI;
                angles.updated_cycles[cycle_phase] = cycle_idx_;
            }
    }

    // Send the data down the pipeline every 4th cycle (30Hz). Can be increased to 120Hz if needed.
    if ((cycle_phase >= 0) ? (cycle_phase == 3) : (cycle_idx_ % 4 == 0)) {
        angles_frame_.time = cycle_start_time_;
        angles_frame_.fix_level = (cycle_phase >= 0 && cycle_fix_level_ >= kCycleFixAcquired)
                                        ? FixLevel::kCycleSynced : FixLevel::kCycleSyncing;
        angles_frame_.cycle_idx = cycle_idx_;
        angles_frame_.phase_id = cycle_phase;
        Producer<SensorAnglesFrame>::produce(angles_frame_);
    }
    
    // Prepare for the next cycle.
    reset_cycle_pulses();
    cycle_start_time_ += cycle_period + pulse_start_corrections[0];
    cycle_idx_++;
}

void PulseProcessor::reset_cycle_pulses() {
    for (int i = 0; i < num_base_stations; i++)
        cycle_long_pulses_[i].clear();
    unclassified_long_pulses_.clear();
    cycle_short_pulses_.clear();
}

void PulseProcessor::do_work(Timestamp cur_time) {
    if (cycle_fix_level_ >= kCycleFixCandidate) {
        if (cur_time - cycle_start_time_ > cycle_processing_point) {
            process_cycle_fix(cur_time);
        }
    } else {  // No fix.
        if (throttle_ms(TimeDelta(1000, ms), cur_time, &cycle_start_time_)) {
            // Send frame showing we have no signals.
            angles_frame_.time = cur_time;
            angles_frame_.fix_level = FixLevel::kNoSignals;
            angles_frame_.cycle_idx = 0;
            angles_frame_.phase_id = 0;
            Producer<SensorAnglesFrame>::produce(angles_frame_);
        }
    }
}

bool PulseProcessor::debug_cmd(HashedWord *input_words) {
    if (phase_classifier_.debug_cmd(input_words))
        return true;
    if (*input_words++ == "pp"_hash)
        switch (*input_words++) {
            case "angles"_hash: return producer_debug_cmd<SensorAnglesFrame>(this, input_words, "SensorAnglesFrame");
            case "bits"_hash: return producer_debug_cmd<DataFrameBit>(this, input_words, "DataFrameBit");
            case "show"_hash: debug_print_state_ = true; return true;
            case "off"_hash: debug_print_state_ = false; return true;
        }
    return false;
}

void PulseProcessor::debug_print(PrintStream &stream) {
    phase_classifier_.debug_print(stream);
    producer_debug_print<SensorAnglesFrame>(this, stream);
    producer_debug_print<DataFrameBit>(this, stream);
    if (debug_print_state_) {
        stream.printf("PulseProcessor: fix %d, cycle id %d, num pulses %d %d %d %d, time from last pulse %d\n", 
            cycle_fix_level_, cycle_idx_, cycle_long_pulses_[0].size(), cycle_long_pulses_[1].size(), 
            cycle_short_pulses_.size(), unclassified_long_pulses_.size(), time_from_last_long_pulse_.get_value(usec));
    }
}


