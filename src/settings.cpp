// This file manages reading/writing data to EEPROM and configuration mode.
#include "settings.h"
#include "primitives/string_utils.h"
#include "vive_sensors_pipeline.h"
#include "led_state.h"

#include <avr_functions.h>
#include <Stream.h>
#include <kinetis.h>

constexpr uint32_t current_settings_version = 0xbabe0000 + sizeof(PersistentSettings);
constexpr uint32_t *eeprom_addr = 0;
/* Example settings
# Comments are prepended by '#'
reset  # Reset all settings to clean state
sensor0 pin 12 positive
base0 origin -1.528180 2.433750 -1.969390 matrix -0.841840 0.332160 -0.425400 -0.046900 0.740190 0.670760 0.537680 0.584630 -0.607540
base1 origin 1.718700 2.543170 0.725060 matrix 0.458350 -0.649590 0.606590 0.028970 0.693060 0.720300 -0.888300 -0.312580 0.336480
object0 sensor0
serial1 57600
stream0 mavlink object0 ned 110 > serial1
stream1 angles > usb_serial
stream2 position object0 > usb_serial
*/

PersistentSettings settings;

PersistentSettings::PersistentSettings() {
    reset();
    read_from_eeprom();
}

bool PersistentSettings::read_from_eeprom() {
    uint32_t eeprom_version = eeprom_read_dword(eeprom_addr);
    if (eeprom_version == current_settings_version) {
        // Normal initialization.
        eeprom_read_block(this, eeprom_addr + 4, sizeof(*this));
        return true;
    } 
    // Unknown version.
    return false;
}

void PersistentSettings::write_to_eeprom() {
    eeprom_write_dword(eeprom_addr, current_settings_version);
    eeprom_write_block(this, eeprom_addr + 4, sizeof(*this));
}

void PersistentSettings::restart_in_configuration_mode() {
    is_configured_ = false;
    write_to_eeprom();
    SCB_AIRCR = 0x5FA0004; // Restart Teensy.
}

// Initialize settings.
void PersistentSettings::reset() {
    memset(this, 0, sizeof(*this));

    // Defaults.
    outputs_.set_size(num_outputs);
    outputs_[0].active = true;
}

bool PersistentSettings::validate_setup(Print &error_stream) {
    try {
        // Try to create the pipeline and then delete it.
        std::unique_ptr<Pipeline> pipeline = create_vive_sensor_pipeline(*this);
        return true;
    }
    catch (const std::exception& e) { // This included bad_alloc, runtime_exception etc.
        error_stream.printf("Validation error: %s\n", e.what());
    }
    catch (...) {
        error_stream.printf("Unknown validation error.\n");
    }
    return false;
}

template<typename T, unsigned arr_len>
void PersistentSettings::set_value(Vector<T, arr_len> &arr, uint32_t idx, HashedWord *input_words, Print& stream) {
    if (idx <= arr.size() && idx < arr_len) {
        T def;
        if (def.parse_def(idx, input_words, stream)) {
            bool push_new = idx == arr.size();
            if (push_new)
                arr.push(def); 
            else
                std::swap(arr[idx], def);

            if (validate_setup(stream)) {
                // Success.
                def.print_def(idx, stream); 
            } else {
                // Validation failed. Undo.
                if (push_new)
                    arr.pop(); 
                else
                    std::swap(arr[idx], def);
            }
        }
    } else
        stream.printf("Index too large. Next available index: %d.\n", arr.size());
}


#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wstack-usage=512"  // Allow slightly higher stack usage for this function.

void PersistentSettings::initialize_from_user_input(Stream &stream) {
    Vector<char, max_input_str_len> input_buf{};
    while (true) {
        stream.print("config> ");
        char *input_cmd = nullptr;
        while (!input_cmd) {
            input_cmd = read_line(stream, &input_buf);

            set_led_state(LedState::kConfigMode);
            update_led_pattern(Timestamp::cur_time());
        } 
        
        HashedWord *input_words = hash_words(input_cmd);
        if (!input_words->word || input_words->word[0] == '#')  // Do nothing on comments and empty lines.
            continue;
        
        uint32_t idx = input_words->idx;
        switch (*input_words++) {
        case "view"_hash:
            // Print all current settings.
            stream.printf("# Current configuration. Copy/paste to save/restore.\n", 
                        inputs_.size(), base_stations_.size());
            stream.printf("reset\n");
            for (uint32_t i = 0; i < inputs_.size(); i++)
                inputs_[i].print_def(i, stream);
            for (uint32_t i = 0; i < base_stations_.size(); i++)
                base_stations_[i].print_def(i, stream);
            for (uint32_t i = 0; i < geo_builders_.size(); i++)
                geo_builders_[i].print_def(i, stream);
            for (uint32_t i = 0; i < outputs_.size(); i++)
                outputs_[i].print_def(i, stream);
            for (uint32_t i = 0; i < formatters_.size(); i++)
                formatters_[i].print_def(i, stream);
            break;
        
        case "sensor#"_hash:
            set_value(inputs_, idx, input_words, stream);
            break;
        
        case "base#"_hash:
            set_value(base_stations_, idx, input_words, stream);
            break;

        case "object#"_hash:
            set_value(geo_builders_, idx, input_words, stream);
            break;
        
        case "stream#"_hash:
            set_value(formatters_, idx, input_words, stream);
            break;

        case "usb_serial"_hash:
        case "serial#"_hash:
            if (idx == (uint32_t)-1) idx = 0;
            set_value(outputs_, idx, input_words, stream);
            break;
        
        case "reset"_hash:
            reset();
            stream.printf("Reset successful.\n");
            break;

        case "reload"_hash:
            if (read_from_eeprom())
                stream.printf("Loaded previous configuration from EEPROM.\n");
            else
                stream.printf("No valid configuration found in EEPROM.\n");
            break;

        case "write"_hash:
            if (!validate_setup(stream)) break;
            is_configured_ = true;
            write_to_eeprom();
            stream.printf("Write to EEPROM successful. Type 'continue' to start using it.\n");
            break;

        case "validate"_hash:
            if (validate_setup(stream))
                stream.printf("Validation successful.\n");
            break;

        case "continue"_hash:
            if (!validate_setup(stream)) break;
            return;

        default:
            stream.printf("Unknown command '%s'. Valid commands: view, <module> <settings>, reset, reload, write, validate, continue.\n", (input_words-1)->word);
        }
    }
}
#pragma GCC diagnostic pop

