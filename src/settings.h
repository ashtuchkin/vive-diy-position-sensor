#pragma once
#include "common.h"
#include "input.h"
#include "geometry.h"
#include <type_traits>

class PersistentSettings {
    static_assert(std::is_pod<InputDef>(), "InputDef must be POD");
    static_assert(std::is_pod<BaseStationGeometryDef>(), "BaseStationGeometryDef must be POD");
public:
    // Data accessors
    inline const Vector<InputDef, max_num_inputs> &inputs() const { return inputs_; }
    inline const Vector<BaseStationGeometryDef, num_base_stations> &base_stations() const { return base_stations_; }
    inline const Vector<GeometryBuilderDef, max_num_inputs> &geo_builders() const { return geo_builders_; }
    // TODO: Output type


    // Settings lifecycle methods
    PersistentSettings();
    bool needs_configuration() { return !is_configured_; }
    void initialize_from_user_input(Stream &interactive_stream);
    void restart_in_configuration_mode();

private:
    bool validate_setup(Print &error_stream);

    template<typename T, unsigned arr_len>
    void set_value(Vector<T, arr_len> &arr, uint32_t idx, HashedWord *input_words, Print& stream);

    void read_from_eeprom();
    void write_to_eeprom();

    // NOTE: This is the same layout as used for EEPROM.
    // NOTE: Increase current_settings_version with each change.
    bool is_configured_;
    Vector<InputDef, max_num_inputs> inputs_;
    Vector<BaseStationGeometryDef, num_base_stations> base_stations_;
    Vector<GeometryBuilderDef, max_num_inputs> geo_builders_;
};

static_assert(sizeof(PersistentSettings) < 1500, "PersistentSettings must fit into eeprom");

// Singleton to access current settings.
extern PersistentSettings settings;
