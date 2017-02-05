#include "outputs.h"
#include <Arduino.h>

SensorAnglesTextOutput::SensorAnglesTextOutput(Print &stream)
    : stream_(&stream) {
}

void SensorAnglesTextOutput::consume(const SensorAnglesFrame& f) {
    // Only print on the last phase.
    if (f.phase_id !=3)
        return;
    
    const char* time = "<time>"; // TODO.
    stream_->printf("ANG\t%s", time);
    for (uint32_t i = 0; i < f.sensors.size(); i++) {
        const SensorAngles &angles = f.sensors[i];
        stream_->printf("\t|");
        for (uint32_t j = 0; j < num_cycle_phases; j++) {
            if (angles.updated_cycles[j] == f.cycle_idx - f.phase_id + j)
                stream_->printf("\t%+1.5f", angles.angles[j]);
            else
                stream_->printf("\t");
        }
    }
}


GeometryTextOutput::GeometryTextOutput(Print &stream, uint32_t object_idx)
    : stream_(&stream), object_idx_(object_idx) {
}

void GeometryTextOutput::consume(const ObjectGeometry& f) {
    const char* time = "<time>"; // TODO.
    stream_->printf("GEO\t%d\t%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", object_idx_, time,
        f.xyz[0], f.xyz[1], f.xyz[2], f.q[0], f.q[1], f.q[2], f.q[3]);
}
