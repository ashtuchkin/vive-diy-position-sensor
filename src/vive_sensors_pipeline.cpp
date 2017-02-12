#include "vive_sensors_pipeline.h"

#include "settings.h"
#include "pulse_processor.h"
#include "input.h"
#include "geometry.h"
#include "outputs.h"
#include "debug_node.h"
#include "data_frame_decoder.h"

#include <avr_emulation.h>
#include <HardwareSerial.h>
#include <usb_serial.h>


std::unique_ptr<Pipeline> create_vive_sensor_pipeline(const PersistentSettings &settings) {
    // Create pipeline itself.
    auto pipeline = std::make_unique<Pipeline>();

    // Append Debug node to make it possible to print what's going on.
    pipeline->emplace_front(new DebugNode(pipeline.get(), Serial));

    // Create central element PulseProcessor.
    auto pulse_processor = pipeline->emplace_back(new PulseProcessor(settings.inputs().size()));

    // Create input nodes as configured.
    for (uint32_t input_idx = 0; input_idx < settings.inputs().size(); input_idx++) {
        auto input_def = settings.inputs()[input_idx];
        auto input_node = pipeline->emplace_front(InputNode::create(input_idx, input_def));
        input_node->connect(pulse_processor);
    }    

    // Create Geometry calculating nodes.
    PointGeometryBuilder *geometry_builder = NULL;
    if (settings.base_stations().size() == 2) {
        uint32_t input_idx = 0;
        geometry_builder = pipeline->emplace_back(new PointGeometryBuilder(settings.base_stations(), input_idx));
        pulse_processor->Producer<SensorAnglesFrame>::connect(geometry_builder);

        for (uint32_t i = 0; i < 2; i++) {
            auto dataframe_decoder = pipeline->emplace_back(new DataFrameDecoder(i));
            pulse_processor->Producer<DataFrameBit>::connect(dataframe_decoder);
        }        
    } else {
        // TODO: Print that there's no geometry.
    }

    // Output Nodes
    // auto sensor_angles_output = pipeline->emplace_back(new SensorAnglesTextOutput(Serial));
    // pulse_processor->Producer<SensorAnglesFrame>::connect(sensor_angles_output);

    if (geometry_builder) {
        Serial1.begin(57600);
        auto mavlink_output = pipeline->emplace_back(new MavlinkGeometryOutput(Serial1));
        geometry_builder->connect(mavlink_output);

        // auto text_output = pipeline->emplace_back(new GeometryTextOutput(Serial, 0));
        // geometry_builder->connect(text_output);
    }

    return pipeline;
}
