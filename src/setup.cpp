#include <Arduino.h>
#include "settings.h"
#include "common.h"
#include "input.h"
#include "input_cmp.h"
#include "pulse_processor.h"
#include "geometry.h"
#include "outputs.h"

void setup() {
    // Initialize common modules.
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize persistent settings interactively from user input, if needed.
    if (settings.needs_configuration()) {
        digitalWrite(LED_BUILTIN, HIGH);
        settings.initialize_from_user_input(Serial);
    }

    // Now create the pipeline, from its center element PulseProcessor.
    PulseProcessor *pulse_processor = new PulseProcessor(settings.inputs().size());
    workforce.add_worker_to_front(pulse_processor);

    // Create input nodes.
    Vector<InputNode *, max_num_inputs> input_nodes;
    for (uint32_t input_idx = 0; input_idx < settings.inputs().size(); input_idx++) {
        char error_message[120];
        const InputDefinition& input_def = settings.inputs()[input_idx];
        InputNode *input_node = NULL;
        switch (input_def.input_type) {
            case kPort: /* TODO */ break; 
            case kFTM: /* TODO */ break;
            // TODO: Check result and report error.
            case kCMP: input_node = InputCmpNode::create(input_idx, input_def, error_message); break;
            case kMaxInputType: break; // TODO: Assert
        }
        workforce.add_worker_to_front(input_node);
        input_node->connect(pulse_processor);
        input_nodes.push(input_node);
    }    

    // Create Geometry calculating nodes.
    PointGeometryBuilder *geometry_builder = NULL;
    if (settings.base_stations().size() == 2) {
        uint32_t input_idx = 0;
        geometry_builder = new PointGeometryBuilder(settings.base_stations(), input_idx);
        workforce.add_worker_to_back(geometry_builder);
        pulse_processor->Producer<SensorAnglesFrame>::connect(geometry_builder);
    } else {
        // TODO: Print that no geometry.
    }

    // Create Output Nodes
    if (geometry_builder) {
        Serial1.begin(57600);
        MavlinkGeometryOutput *mavlink_output = new MavlinkGeometryOutput(Serial1);
        geometry_builder->connect(mavlink_output);
        workforce.add_worker_to_back(mavlink_output);

        GeometryTextOutput *text_output = new GeometryTextOutput(Serial, 0);
        geometry_builder->connect(text_output);
        workforce.add_worker_to_back(text_output);
    }

    // After the pipeline is ready, start all inputs.
    for (uint32_t i = 0; i < input_nodes.size(); i++)
        input_nodes[i]->start();
}

// Replace stock yield to make loop() faster.
void yield(void) { }

