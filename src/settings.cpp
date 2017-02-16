// This file manages reading/writing data to EEPROM and configuration mode.
#include "settings.h"
#include "primitives/string_utils.h"
#include "vive_sensors_pipeline.h"
#include "led_state.h"

#include <avr_functions.h>
#include <Stream.h>
#include <kinetis.h>

constexpr uint32_t current_settings_version = 0xbabe0002;
constexpr uint32_t *eeprom_addr = 0;
/* Example settings
reset
i0 pin 12 positive
b0 origin -1.528180 2.433750 -1.969390 matrix -0.841840 0.332160 -0.425400 -0.046900 0.740190 0.670760 0.537680 0.584630 -0.607540
b1 origin 1.718700 2.543170 0.725060 matrix 0.458350 -0.649590 0.606590 0.028970 0.693060 0.720300 -0.888300 -0.312580 0.336480
*/

PersistentSettings settings;

PersistentSettings::PersistentSettings() : is_configured_(false) {
    read_from_eeprom();
}

void PersistentSettings::read_from_eeprom() {
    uint32_t eeprom_version = eeprom_read_dword(eeprom_addr);
    if (eeprom_version == current_settings_version) {
        // Normal initialization.
        eeprom_read_block(this, eeprom_addr + 4, sizeof(*this));
    } else {
        // Unknown version: initialize with zeros.
        memset(this, 0, sizeof(*this));
    }
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


bool PersistentSettings::validate_setup(Print &error_stream) {
    try {
        // Try to create the pipeline and then delete it.
        std::unique_ptr<Pipeline> pipeline = create_vive_sensor_pipeline(*this);
        return true;
    }
    catch (const std::exception& e) { // This included bad_alloc, runtime_exception etc.
        error_stream.printf("Validation error: %s", e.what());
    }
    catch (...) {
        error_stream.printf("Unknown validation error.");
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
        stream.printf("Index too large. Next available index: i%d.\n", arr.size());
}


void PersistentSettings::initialize_from_user_input(Stream &stream) {
    while (true) {
        stream.print("config> ");
        char *input_cmd = nullptr;
        while (!input_cmd) {
            input_cmd = read_line(stream);

            set_led_state(kConfigMode);
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
            break;
        
        case "i#"_hash:
            set_value(inputs_, idx, input_words, stream);
            break;
        
        case "b#"_hash:
            set_value(base_stations_, idx, input_words, stream);
            break;

        case "g#"_hash:
            set_value(geo_builders_, idx, input_words, stream);
        
        case "reset"_hash:
            inputs_.clear();
            base_stations_.clear();
            stream.printf("Reset successful.\n");
            break;

        case "reload"_hash:
            read_from_eeprom();
            stream.printf("Loaded previous configuration from EEPROM.\n");
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




