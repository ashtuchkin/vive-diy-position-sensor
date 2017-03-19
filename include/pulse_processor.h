#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/vector.h"
#include "messages.h"
#include "cycle_phase_classifier.h"

// This node processes Pulses from several sensors, tries to match them to cycle structure and
// output matched set of angles (SensorAnglesFrame) and data bits (DataFrameBit).
class PulseProcessor
    : public WorkerNode
    , public Consumer<Pulse>
    , public Producer<SensorAnglesFrame>
    , public Producer<DataFrameBit> {
public:
    PulseProcessor(uint32_t num_inputs);
    virtual void consume(const Pulse& p);
    virtual void do_work(Timestamp cur_time);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    void process_long_pulse(const Pulse &p);
    void process_short_pulse(const Pulse &p);
    void process_cycle_fix(Timestamp cur_time);
    void reset_cycle_pulses();

    uint32_t num_inputs_;

    // Fix level - increased when everything's right (up to a limit); decreases on errors. See CycleFixLevels
    int cycle_fix_level_;
    
    // Current cycle params
    Timestamp cycle_start_time_;  // Current cycle start time.  TODO: We should track cycle period more precisely.
    uint32_t cycle_idx_;          // Index of current cycle.

    // Classified pulses for current cycle: long (x2, by base stations), short, unclassified.
    Vector<Pulse, max_num_inputs> cycle_long_pulses_[num_base_stations];
    Vector<Pulse, max_num_inputs*4> cycle_short_pulses_;
    Vector<Pulse, max_num_inputs*4> unclassified_long_pulses_;

    // Phase classifier - helps determine which of the 4 cycles in we have now.
    CyclePhaseClassifier phase_classifier_;

    // Output data: angles.
    SensorAnglesFrame angles_frame_;

    TimeDelta time_from_last_long_pulse_;
    bool debug_print_state_;
};
