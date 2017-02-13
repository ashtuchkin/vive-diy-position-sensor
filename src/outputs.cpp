#include "outputs.h"
#include <Arduino.h>

SensorAnglesTextOutput::SensorAnglesTextOutput(Print &stream)
    : stream_(&stream)
    , debug_disable_output_(false) {
}

void SensorAnglesTextOutput::consume(const SensorAnglesFrame& f) {
    // Only print on the last phase.
    if (f.phase_id != 3 || debug_disable_output_)
        return;

    auto time = f.time.get_value(msec);
    stream_->printf("ANG\t%d", time);
    for (uint32_t i = 0; i < f.sensors.size(); i++) {
        const SensorAngles &angles = f.sensors[i];
        for (uint32_t j = 0; j < num_cycle_phases; j++) {
            if (angles.updated_cycles[j] == f.cycle_idx - f.phase_id + j)
                stream_->printf("\t%.4f", angles.angles[j]);
            else
                stream_->print("\t");
        }
    }
    stream_->println();
}

/* Use following command to disable outputs:
angles_out off
geo_out0 off
*/
bool SensorAnglesTextOutput::debug_cmd(HashedWord *input_words) {
    if (*input_words++ == "angles_out"_hash) {
        switch(*input_words++) {
        case "off"_hash: debug_disable_output_ = true; return true;
        case "on"_hash: debug_disable_output_ = false; return true;
        }
    }
    return false;
}


GeometryTextOutput::GeometryTextOutput(Print &stream, uint32_t object_idx)
    : stream_(&stream)
    , object_idx_(object_idx)
    , debug_disable_output_(false) {
}

void GeometryTextOutput::consume(const ObjectGeometry& f) {
    if (debug_disable_output_)
        return;

    auto time = f.time.get_value(msec);
    stream_->printf("GEO%d\t%u\t%.4f\t%.4f\t%.4f", object_idx_, time, f.xyz[0], f.xyz[1], f.xyz[2]);
    if (f.q[0] != 1.0f) {
        // Output quaternion if available.
        stream_->printf("\t%.4f\t%.4f\t%.4f\t%.4f\n", f.q[0], f.q[1], f.q[2], f.q[3]);
    } else {
        stream_->println();
    }
}

bool GeometryTextOutput::debug_cmd(HashedWord *input_words) {
    if (*input_words == "geo_out#"_hash && input_words->idx == object_idx_) {
        input_words++;
        switch(*input_words++) {
        case "off"_hash: debug_disable_output_ = true; return true;
        case "on"_hash: debug_disable_output_ = false; return true;
        }
    }
    return false;
}
