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
    auto debug_node = pipeline->emplace_front(std::make_unique<DebugNode>(pipeline.get(), Serial));

    // Create central element PulseProcessor.
    auto pulse_processor = pipeline->emplace_back(std::make_unique<PulseProcessor>(settings.inputs().size()));

    // Create input nodes as configured.
    Vector<InputNode *, max_num_inputs> input_nodes{};
    for (uint32_t input_idx = 0; input_idx < settings.inputs().size(); input_idx++) {
        auto input_def = settings.inputs()[input_idx];
        auto input_node = pipeline->emplace_front(InputNode::create(input_idx, input_def));
        input_node->connect(pulse_processor);
        input_nodes.push(input_node);
    }    

    // Create geometry builders as configured.
    Vector<GeometryBuilder *, max_num_inputs> geometry_builders{};
    if (settings.geo_builders().size() > 0 && settings.base_stations().size() != 2)
        throw_printf("2 base stations must be defined to use geometry builders.");
    
    for (uint32_t geo_idx = 0; geo_idx < settings.geo_builders().size(); geo_idx++) {
        auto geometry_builder = pipeline->emplace_back(
            std::make_unique<PointGeometryBuilder>(geo_idx, settings.geo_builders()[geo_idx], settings.base_stations())
        );
        pulse_processor->Producer<SensorAnglesFrame>::connect(geometry_builder);
        geometry_builders.push(geometry_builder);
    }

    // Create Data Frame Decoders for all defined base stations.
    for (uint32_t base_idx = 0; base_idx < settings.base_stations().size(); base_idx++) {
        auto dataframe_decoder = pipeline->emplace_back(std::make_unique<DataFrameDecoder>(base_idx));
        pulse_processor->Producer<DataFrameBit>::connect(dataframe_decoder);
    }        

    // Output Nodes
    // auto sensor_angles_output = pipeline->emplace_back(
    //          std::make_unique<SensorAnglesTextOutput>(debug_node->stream()));
    // pulse_processor->Producer<SensorAnglesFrame>::connect(sensor_angles_output);

    if (geometry_builders.size() > 0) {
        auto geometry_builder = geometry_builders[0];
        Serial1.begin(57600);

        // TODO: Make configurable.
        auto coord_converter = pipeline->emplace_back(CoordinateSystemConverter::NED(110.f));
        geometry_builder->connect(coord_converter);

        auto mavlink_output = pipeline->emplace_back(std::make_unique<MavlinkGeometryOutput>(Serial1));
        coord_converter->connect(mavlink_output);

        auto text_output = pipeline->emplace_back(std::make_unique<GeometryTextOutput>(debug_node->stream(), 0));
        geometry_builder->connect(text_output);
    }

    return pipeline;
}
