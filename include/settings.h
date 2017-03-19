#pragma once
#include "messages.h"
#include "input.h"
#include "geometry.h"
#include "formatters.h"
#include "outputs.h"
#include <type_traits>

// This class provides configurability to our project. It reads/writes configuration data to EEPROM and provides
// configuration command interface.
class PersistentSettings {
public:
    // Data accessors
    inline const Vector<InputDef, max_num_inputs> &inputs() const { return inputs_; }
    inline const Vector<BaseStationGeometryDef, num_base_stations> &base_stations() const { return base_stations_; }
    inline const Vector<GeometryBuilderDef, max_num_inputs> &geo_builders() const { return geo_builders_; }
    inline const Vector<FormatterDef, max_num_inputs> &formatters() const { return formatters_; }
    inline const Vector<OutputDef, num_outputs> &outputs() const { return outputs_; }

    // Settings lifecycle methods
    PersistentSettings();
    bool needs_configuration() { return !is_configured_; }
    void restart_in_configuration_mode();

    std::unique_ptr<Pipeline> create_configuration_pipeline(uint32_t stream_idx);
    bool process_command(char *input_cmd, PrintStream &stream);

private:
    bool validate_setup(PrintStream &error_stream);

    template<typename T, unsigned arr_len>
    void set_value(Vector<T, arr_len> &arr, uint32_t idx, HashedWord *input_words, PrintStream &stream);

    void reset();
    bool read_from_eeprom();
    void write_to_eeprom();

    // NOTE: This is the same layout as used for EEPROM.
    // NOTE: Increase current_settings_version with each change.
    bool is_configured_;
    Vector<InputDef, max_num_inputs> inputs_;
    Vector<BaseStationGeometryDef, num_base_stations> base_stations_;
    Vector<GeometryBuilderDef, max_num_inputs> geo_builders_;
    Vector<FormatterDef, max_num_inputs> formatters_;
    Vector<OutputDef, num_outputs> outputs_;
};

// Singleton to access current settings.
extern PersistentSettings settings;

// Functions to be implemented by platform
void restart_system();
void eeprom_read(uint32_t eeprom_addr, void *dest, uint32_t len);
void eeprom_write(uint32_t eeprom_addr, const void *src, uint32_t len);
