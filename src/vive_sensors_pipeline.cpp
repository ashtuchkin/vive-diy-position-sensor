#include "vive_sensors_pipeline.h"

#include "settings.h"
#include "pulse_processor.h"
#include "input_cmp.h"
#include "geometry.h"
#include "outputs.h"
#include "debug_node.h"

#include <avr_emulation.h>
#include <HardwareSerial.h>
#include <usb_serial.h>


std::unique_ptr<Pipeline> create_vive_sensor_pipeline(const PersistentSettings &settings) {
    auto pipeline = std::make_unique<Pipeline>();

    // Append Debug node to make it possible to print what's going on.
    pipeline->emplace_front(new DebugNode(pipeline.get(), Serial));

    // Create center element PulseProcessor.
    auto pulse_processor = pipeline->emplace_back(new PulseProcessor(settings.inputs().size()));

    // Create input nodes.
    for (uint32_t input_idx = 0; input_idx < settings.inputs().size(); input_idx++) {
        char error_message[120];
        const InputDefinition& input_def = settings.inputs()[input_idx];
        InputNode *input_node = NULL;
        switch (input_def.input_type) {
            case kPort: break;  // TODO
            case kFTM: break;  // TODO
            // TODO: Check result and report error.
            case kCMP: input_node = InputCmpNode::create(input_idx, input_def, error_message); break;
            case kMaxInputType: break; // TODO: Assert
        }
        pipeline->emplace_front(input_node);
        input_node->connect(pulse_processor);
    }    

    // Create Geometry calculating nodes.
    PointGeometryBuilder *geometry_builder = NULL;
    if (settings.base_stations().size() == 2) {
        uint32_t input_idx = 0;
        geometry_builder = pipeline->emplace_back(new PointGeometryBuilder(settings.base_stations(), input_idx));
        pulse_processor->Producer<SensorAnglesFrame>::connect(geometry_builder);
    } else {
        // TODO: Print that there's no geometry.
    }

    // Create Output Nodes
    if (geometry_builder) {
        Serial1.begin(57600);
        auto mavlink_output = pipeline->emplace_back(new MavlinkGeometryOutput(Serial1));
        geometry_builder->connect(mavlink_output);

        auto text_output = pipeline->emplace_back(new GeometryTextOutput(Serial, 0));
        geometry_builder->connect(text_output);
    }

    return pipeline;
}
