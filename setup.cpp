#include "main.h"
#include "settings.h"

void setup() {
    // Initialize common modules.
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize persistent settings interactively from user input, if needed.
    if (settings.needs_configuration()) {
        digitalWrite(LED_BUILTIN, HIGH);
        settings.initialize_from_user_input(Serial);
    }

    // Initialize inputs.
    char error_message[120];
    for (uint32_t input_idx = 0; input_idx < settings.input_count; input_idx++) {
        const InputDefinition& input_def = settings.inputs[input_idx];
        switch (input_def.input_type) {
            case kPort: /* TODO */ break; 
            case kFTM: /* TODO */ break;
            case kCMP: setupCmpInput(input_idx, input_def, error_message); break; // TODO: Check result and report error.
            default: break; // Skip
        }
    }

    // Initialize outputs (TODO)
    Serial1.begin(57600);
}

// Replace stock yield to make loop() faster.
void yield(void) { }

