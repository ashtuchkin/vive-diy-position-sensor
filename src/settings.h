#pragma once
#include "common.h"
#include "input.h"
#include "geometry.h"
class Stream;

class PersistentSettings {
public:
    // Data accessors
    inline const Vector<InputDefinition, max_num_inputs> &inputs() const { return inputs_; }
    inline const Vector<BaseStationGeometry, num_base_stations> &base_stations() const { return base_stations_; }
    // TODO: Output type


    // Settings lifecycle methods
    PersistentSettings();
    bool needs_configuration() { return !is_configured_; }
    void initialize_from_user_input(Stream &interactive_stream);
    void restart_in_configuration_mode();

private:
    bool validate_input_def(uint32_t idx, const InputDefinition &inp_def, Stream &stream);
    void read_from_eeprom();
    void write_to_eeprom();

    // NOTE: This is the same layout as used for EEPROM.
    // NOTE: Increase current_settings_version with each change.
    bool is_configured_;
    Vector<InputDefinition, max_num_inputs> inputs_;
    Vector<BaseStationGeometry, num_base_stations> base_stations_;
};

// Singleton to access current settings.
extern PersistentSettings settings;
