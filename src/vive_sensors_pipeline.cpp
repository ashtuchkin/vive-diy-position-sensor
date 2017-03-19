#include "vive_sensors_pipeline.h"

#include "data_frame_decoder.h"
#include "debug_node.h"
#include "formatters.h"
#include "geometry.h"
#include "input.h"
#include "outputs.h"
#include "pulse_processor.h"

#include <vector>

// Create Pipeline specialized for Vive Sensors, using provided configuration settings.
// In this function we create and interconnect all needed WorkerNodes to make project work.
// NOTE: This function will also be called for validation purposes, so no hardware changes should be made.
// (move all hardware changes to start() methods)
std::unique_ptr<Pipeline> create_vive_sensor_pipeline(const PersistentSettings &settings) {

    // Create pipeline itself.
    auto pipeline = std::make_unique<Pipeline>();

    // Create central element PulseProcessor.
    auto pulse_processor = pipeline->add_back(std::make_unique<PulseProcessor>(settings.inputs().size()));

    // Create input nodes as configured.
    std::vector<InputNode *> inputs;
    for (uint32_t i = 0; i < settings.inputs().size(); i++) {
        auto &def = settings.inputs()[i];
        auto node = pipeline->add_front(InputNode::create(i, def));
        node->pipe(pulse_processor);
        inputs.push_back(node);
    }    

    // Create geometry builders as configured.
    std::vector<GeometryBuilder *> geometry_builders;
    for (uint32_t i = 0; i < settings.geo_builders().size(); i++) {
        auto &def = settings.geo_builders()[i];
        auto node = pipeline->add_back(std::make_unique<PointGeometryBuilder>(i, def, settings.base_stations()));
        pulse_processor->Producer<SensorAnglesFrame>::pipe(node);
        geometry_builders.push_back(node);
    }

    // Create Data Frame Decoders for all defined base stations.
    for (uint32_t i = 0; i < settings.base_stations().size(); i++) {
        auto node = pipeline->add_back(std::make_unique<DataFrameDecoder>(i));
        pulse_processor->Producer<DataFrameBit>::pipe(node);
    }

    // Create Output Nodes
    std::vector<std::unique_ptr<OutputNode>> output_nodes(num_outputs);
    for (uint32_t i = 0; i < settings.outputs().size(); i++) {
        auto &def = settings.outputs()[i];
        if (def.active) {
            output_nodes[i] = OutputNode::create(i, def);
            // NOTE: We defer adding node to the pipeline until after formatter nodes.
        }
    }

    // Create Formatter Nodes
    for (uint32_t i = 0; i < settings.formatters().size(); i++) {
        auto &def = settings.formatters()[i];
        FormatterNode *formatter;
        switch (def.formatter_type) {
            case FormatterType::kAngles: {
                auto node = pipeline->add_back(std::make_unique<SensorAnglesTextFormatter>(i, def));
                pulse_processor->Producer<SensorAnglesFrame>::pipe(node);
                formatter = node;
                break;
            }
            // TODO: case FormatterType::kDataFrame:
            case FormatterType::kPosition: {
                if (def.input_idx >= geometry_builders.size())
                    throw_printf("Geometry builder g%d not found.", def.input_idx);
                Producer<ObjectPosition> *geometry_source = geometry_builders[def.input_idx];

                // Convert coordinate system if needed.
                if (auto coord_conv = CoordinateSystemConverter::create(def.coord_sys_type, def.coord_sys_params)) {
                    auto node = pipeline->add_back(std::move(coord_conv));
                    geometry_source->pipe(node); 
                    geometry_source = node;
                }

                // Instantiate the concrete subtype of a geometry formatter.
                auto node = pipeline->add_back(GeometryFormatter::create(i, def));
                geometry_source->pipe(node); 
                formatter = node;
                break;
            }
            default: 
                throw_printf("Unknown formatter type: %d", def.formatter_type);
        }

        // pipe formatter to the output.
        if (def.output_idx < output_nodes.size() && output_nodes[def.output_idx])
            formatter->pipe(output_nodes[def.output_idx].get());
        else
            throw_printf("Uninitialized output %d given for stream %d", def.output_idx, i);
    }

    // Add Output Nodes to pipeline. It's preferable to do it last to keep the order of execution straight.
    std::vector<OutputNode *> outputs(num_outputs);
    for (uint32_t i = 0; i < output_nodes.size(); i++)
        if (output_nodes[i])
            outputs[i] = pipeline->add_back(std::move(output_nodes[i]));

    // Append Debug node to make it possible to print what's going on.
    // TODO: Make it configurable which output to pipe to.
    if (OutputNode *debug_output = outputs[0]) {
        auto debug_node = pipeline->add_back(std::make_unique<DebugNode>(pipeline.get()));
        debug_node->Producer<DataChunk>::pipe(debug_output);
        debug_node->Producer<OutputCommand>::pipe(debug_output);
        debug_output->pipe(debug_node);
    }

    return pipeline;
}
