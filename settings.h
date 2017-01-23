#pragma once
#include "main.h"

struct InputDefinition {
    uint32_t pin;  // Teensy PIN number
    bool pulse_polarity; // true = Positive, false = Negative.
    InputType input_type;
    uint32_t initial_cmp_threshold;
};

struct BaseStationGeometry {
    float mat[9];    // Normalized rotation matrix.
    float origin[3]; // Origin point
};

class PersistentSettings {
public:
    // NOTE: This is the same layout as used for EEPROM.
    // NOTE: Increase current_settings_version with each change.
    bool is_configured;
    uint32_t input_count;
    uint32_t base_station_count;
    InputDefinition inputs[max_num_inputs];
    BaseStationGeometry base_stations[max_num_base_stations];
    // TODO: Output type

    PersistentSettings();
    bool needs_configuration();
    void initialize_from_user_input(Stream &interactive_stream);
    void restart_in_configuration_mode();

private:
    bool validate_input_def(uint32_t idx, const InputDefinition &inp_def, Stream &stream);
    void read_from_eeprom();
    void write_to_eeprom();
};

// Singleton to access current settings.
extern PersistentSettings settings;

