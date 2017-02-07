// This file manages reading/writing data to EEPROM and configuration mode.
#include "settings.h"
#include <Arduino.h>
#include "input_cmp.h"
#include "primitives/string_utils.h"

constexpr uint32_t current_settings_version = 0xbabe0001;
constexpr uint32_t * eeprom_addr = 0;
/* Example settings
reset
i0 pin 12 positive
b0 origin -1.528180 2.433750 -1.969390 matrix -0.841840 0.332160 -0.425400 -0.046900 0.740190 0.670760 0.537680 0.584630 -0.607540
b1 origin 1.718700 2.543170 0.725060 matrix 0.458350 -0.649590 0.606590 0.028970 0.693060 0.720300 -0.888300 -0.312580 0.336480
*/

PersistentSettings settings;

PersistentSettings::PersistentSettings() {
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


uint32_t input_type_names[kMaxInputType] = {"cmp"_hash, "ftm"_hash, "port_irq"_hash};

void print_input_def(uint32_t idx, const InputDefinition &inp_def, Stream &stream) {
    stream.printf("i%d pin %d %s %s", idx, inp_def.pin, inp_def.pulse_polarity ? "positive" : "negative", input_type_names[inp_def.input_type]);
    if (inp_def.input_type == kCMP)
        stream.printf(" %d", inp_def.initial_cmp_threshold);
    stream.println();
}

bool parse_input_def(HashedWord *input_words, InputDefinition *inp_def, Stream &stream) {
    if (*input_words == "pin"_hash)
        input_words++; // Ignore "pin" word
    
    if (!input_words->as_uint32(&inp_def->pin) || inp_def->pin >= CORE_NUM_TOTAL_PINS) {
        stream.printf("Invalid/missing pin number\n"); return false;
    }
    input_words++;

    if (!input_words->word) {
        stream.printf("Missing polarity\n"); return false;
    } else if (*input_words == "positive"_hash) {
        inp_def->pulse_polarity = true;
    } else if (*input_words == "negative"_hash) {
        inp_def->pulse_polarity = false;
    } else {
        stream.printf("Unknown polarity: %s; Only 'positive' and 'negative' supported.\n", input_words->word); return false;
    }
    input_words++;

    if (!input_words->word) {
        // Use default input type: Comparator
        inp_def->input_type = kCMP;
    } else {
        int i; 
        for (i = 0; i < kMaxInputType; i++)
            if (*input_words == input_type_names[i]) {
                inp_def->input_type = (InputType)i;
                break;
            }
        if (i == kMaxInputType) {
            stream.printf("Unknown input type. Supported types: 'port_irq', 'ftm', 'cmp'.\n"); return false;
        }
    }
    input_words++;

    if (inp_def->input_type == kCMP) {
        // For comparators, also read initial threshold level.
        if (!input_words->word) {
            inp_def->initial_cmp_threshold = 20; // Default threshold level.
        } else if (!input_words->as_uint32(&inp_def->initial_cmp_threshold) || inp_def->initial_cmp_threshold >= 64) {
            stream.printf("Invalid threshold level. Supported values: 0-63.\n"); return false;
        }
    } else {
        stream.printf("ftm and port_irq input types are not supported yet.\n"); return false;
    }
    return true;
}

bool PersistentSettings::validate_input_def(uint32_t idx, const InputDefinition &inp_def, Stream &stream) {
    char error_message[120];
    bool is_valid = true;
    uint32_t len = (idx == inputs_.size()) ? idx+1 : inputs_.size();
    for (uint32_t input_idx = 0; input_idx < len; input_idx++) {
        const InputDefinition& input_def = (input_idx == idx) ? inp_def : inputs_[input_idx];
        switch (input_def.input_type) {
            case kPort: /* TODO */ break; 
            case kFTM: /* TODO */ break;
            case kCMP:
                if (!InputCmpNode::create(input_idx, input_def, error_message)) { 
                    stream.printf("Error: %s", error_message); 
                    is_valid = false;
                }; 
                break;
            default: break; // Skip
        }
    }
    InputCmpNode::reset_all();
    return is_valid;
}

void print_base_station(uint32_t idx, const BaseStationGeometry& station, Stream &stream) {
    stream.printf("b%d origin", idx);
    for (int j = 0; j < 3; j++)
        stream.printf(" %f", station.origin[j]);
    stream.printf(" matrix");
    for (int j = 0; j < 9; j++)
        stream.printf(" %f", station.mat[j]);
    stream.println();
}

bool parse_base_station(HashedWord *input_words, BaseStationGeometry *pStation, Stream &stream) {
    if (*input_words == "origin"_hash)
        input_words++;
    for (int i = 0; i < 3; i++, input_words++)
        if (!input_words->as_float(&pStation->origin[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    if (*input_words == "matrix"_hash)
        input_words++;
    for (int i = 0; i < 9; i++, input_words++)
        if (!input_words->as_float(&pStation->mat[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    return true;
}

void PersistentSettings::initialize_from_user_input(Stream &stream) {
    while (true) {
        stream.print("> ");
        char *input_cmd = nullptr;
        while (!input_cmd)
            input_cmd = read_line(stream);
        HashedWord *input_words = hash_words(parse_words(input_cmd));

        if (!input_words->word || input_words->word[0] == '#')  // Do nothing on comments and empty lines.
            continue;
        
        uint32_t idx = input_words->idx;
        switch (*input_words++) {
        case "view"_hash:
            // Print all current settings.
            stream.printf("# Current configuration: %d inputs, %d base stations; copy/paste to save/restore.\n", 
                        inputs_.size(), base_stations_.size());
            stream.printf("reset\n");
            for (uint32_t i = 0; i < inputs_.size(); i++)
                print_input_def(i, inputs_[i], stream);
            for (uint32_t i = 0; i < base_stations_.size(); i++)
                print_base_station(i, base_stations_[i], stream);
            break;
        
        case "i#"_hash:
            // Set definition for input i<n>
            if (idx <= inputs_.size() && idx < max_num_inputs) {
                InputDefinition inp_def;
                if (parse_input_def(input_words, &inp_def, stream) && validate_input_def(idx, inp_def, stream)) {
                    if (idx < inputs_.size())
                        inputs_[idx] = inp_def;
                    else
                        inputs_.push(inp_def);
                    print_input_def(idx, inputs_[idx], stream);
                }
            } else
                stream.printf("Input index too large. Next available index: i%d.\n", inputs_.size());
            break;
        
        case "b#"_hash:
            // Set definition for base station b<n>
            if (idx <= base_stations_.size() && idx < num_base_stations) {
                BaseStationGeometry base_station;
                if (parse_base_station(input_words, &base_station, stream)) {
                    if (idx < base_stations_.size())
                        base_stations_[idx] = base_station;
                    else
                        base_stations_.push(base_station);
                    print_base_station(idx, base_stations_[idx], stream);
                }
            } else
                stream.printf("Base station index too large. Next available index: b%d.\n", base_stations_.size());
            break;
        
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
            is_configured_ = true;
            write_to_eeprom();
            stream.printf("Write to EEPROM successful. Type 'continue' to start using it.\n");
            break;

        case "continue"_hash:
            return;

        default:
            stream.printf("Unknown command '%s'. Valid commands: view, <module> <settings>, reset, reload, write, continue.\n", (input_words-1)->word);
        }
    }
}




