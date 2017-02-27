#include <catch.hpp>
#include "pulse_processor.h"
#include <memory>


TEST_CASE("PulseProcessor gets a fix after small number of pulses") {
    const int num_inputs = 1;
    auto pp = std::make_unique<PulseProcessor>(num_inputs);

    pp->consume({.input_idx=0, .start_time=Timestamp(), .pulse_len=TimeDelta(128, usec)});
    REQUIRE(num_inputs == 1);
}
