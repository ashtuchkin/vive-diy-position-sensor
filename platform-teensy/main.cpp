#include "vive_sensors_pipeline.h"
#include "settings.h"

extern "C" int main() {
    // Initialize persistent settings interactively from user input, if needed.
    if (settings.needs_configuration()) {
        settings.create_configuration_pipeline(0)->run();
    }

    // Create worker nodes pipeline from settings, 
    auto pipeline = create_vive_sensor_pipeline(settings);

    // Register & start input nodes' interrupts,
    // Process pulses, cycles and output data.
    pipeline->run();
}
