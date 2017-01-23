// This file manages reading/writing data to EEPROM and configuration mode.
#include "settings.h"

const static uint32_t current_settings_version = 0xbabe0001;
uint32_t *eeprom_addr = 0;
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

bool PersistentSettings::needs_configuration() {
    return !is_configured;
}

void PersistentSettings::restart_in_configuration_mode() {
    is_configured = false;
    write_to_eeprom();
    SCB_AIRCR = 0x5FA0004; // Restart Teensy.
}

// Returns pointer to a null-terminated next word in *str (or NULL); updates *str to point to remaining string.
char* next_word(char **str) {
    char *start = *str;
    if (start == NULL)
        return NULL;
    while (*start != 0 && *start <= ' ')
        start++;
    if (*start == 0) {
        *str = NULL;
        return NULL;
    }
    char *end = start+1;
    while (*end > ' ')
        end++;
    if (*end != 0) {
        *end = 0;
        *str = end+1;
    } else {
        *str = NULL;
    }
    return start;
}

// Parses given string into a uint32 and returns if the parsing is successful.
bool parse_uint32(const char *str, uint32_t *res) {
    if (*str == 0)
        return false;
    char *endparse;
    *res = strtoul(str, &endparse, 10);
    return (*endparse == 0);
}

bool parse_float(const char *str, float *res) {
    if (*str == 0)
        return false;
    char *endparse;
    *res = strtof(str, &endparse);
    return (*endparse == 0);
}

const char* input_type_names[kMaxInputType] = {"cmp", "ftm", "port_irq"};

void print_input_def(uint32_t idx, const InputDefinition &inp_def, Stream &stream) {
    stream.printf("i%d pin %d %s %s", idx, inp_def.pin, inp_def.pulse_polarity ? "positive" : "negative", input_type_names[inp_def.input_type]);
    if (inp_def.input_type == kCMP)
        stream.printf(" %d", inp_def.initial_cmp_threshold);
    stream.println();
}

bool parse_input_def(char *input_string, InputDefinition *inp_def, Stream &stream) {
    char *pin_str = next_word(&input_string);
    if (!strcmp(pin_str, "pin"))
        pin_str = next_word(&input_string); // Ignore "pin" word
    
    if (!parse_uint32(pin_str, &inp_def->pin) || inp_def->pin >= CORE_NUM_TOTAL_PINS) {
        stream.printf("Invalid/missing pin number\n"); return false;
    }

    char *polarity = next_word(&input_string);
    if (!polarity) {
        stream.printf("Missing polarity\n"); return false;
    } else if (!strcmp(polarity, "positive")) {
        inp_def->pulse_polarity = true;
    } else if (!strcmp(polarity, "negative")) {
        inp_def->pulse_polarity = false;
    } else {
        stream.printf("Unknown polarity: %s; Only 'positive' and 'negative' supported.\n", polarity); return false;
    }

    char *type = next_word(&input_string);
    if (!type) {
        // Use default input type: Comparator
        inp_def->input_type = kCMP;
    } else {
        int i; 
        for (i = 0; i < kMaxInputType; i++)
            if (!strcmp(type, input_type_names[i])) {
                inp_def->input_type = (InputType)i;
                break;
            }
        if (i == kMaxInputType) {
            stream.printf("Unknown input type. Supported types: 'port_irq', 'ftm', 'cmp'.\n"); return false;
        }
    }

    if (inp_def->input_type == kCMP) {
        // For comparators, also read initial threshold level.
        char *threshold_str = next_word(&input_string);
        if (!threshold_str) {
            inp_def->initial_cmp_threshold = 20; // Default threshold level.
        } else if (!parse_uint32(threshold_str, &inp_def->initial_cmp_threshold) || inp_def->initial_cmp_threshold >= 64) {
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
    uint32_t len = (idx == input_count) ? idx+1 : input_count;
    for (uint32_t input_idx = 0; input_idx < len; input_idx++) {
        const InputDefinition& input_def = (input_idx == idx) ? inp_def : inputs[input_idx];
        switch (input_def.input_type) {
            case kPort: /* TODO */ break; 
            case kFTM: /* TODO */ break;
            case kCMP:
                if (!setupCmpInput(input_idx, input_def, error_message, /*validation_mode=*/true)) { 
                    stream.printf("Error: %s", error_message); 
                    is_valid = false;
                }; 
                break;
            default: break; // Skip
        }
    }
    resetCmpAfterValidation();
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

bool parse_base_station(char *input_string, BaseStationGeometry *pStation, Stream &stream) {
    char *str = next_word(&input_string);
    if (!strcmp(str, "origin"))
        str = next_word(&input_string);
    for (int i = 0; i < 3; i++, str = next_word(&input_string))
        if (!parse_float(str, &pStation->origin[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    if (!strcmp(str, "matrix"))
        str = next_word(&input_string);
    for (int i = 0; i < 9; i++, str = next_word(&input_string))
        if (!parse_float(str, &pStation->mat[i])) {
            stream.printf("Invalid base station format\n"); return false;
        }
    return true;
}

void PersistentSettings::initialize_from_user_input(Stream &stream) {
    stream.setTimeout(1000000000);

    char input_string_buf[1000] = {0};
    uint32_t idx = 0;
    while (true) {
        stream.print("> ");
        stream.readBytesUntil('\n', input_string_buf, sizeof(input_string_buf));
        char *input_string = input_string_buf;
        const char *command = next_word(&input_string);

        if (!command || command[0] == '#') {
            // Do nothing.

        } else if (!strcmp(command, "view")) {
            // Print all current settings.
            stream.printf("# Current configuration: %d inputs, %d base stations; copy/paste to save/restore.\n", input_count, base_station_count);
            stream.printf("reset\n");
            for (uint32_t i = 0; i < input_count; i++) {
                print_input_def(i, inputs[i], stream);
            }
            for (uint32_t i = 0; i < base_station_count; i++) {
                print_base_station(i, base_stations[i], stream);
            }

        } else if (command[0] == 'i' && parse_uint32(command+1, &idx)) {
            // Set definition for input i<n>
            if (idx <= input_count && idx < max_num_inputs) {
                InputDefinition inp_def;
                if (parse_input_def(input_string, &inp_def, stream) && validate_input_def(idx, inp_def, stream)) {
                    inputs[idx] = inp_def;
                    if (idx == input_count)
                        input_count = idx+1;
                    print_input_def(idx, inputs[idx], stream);
                }
            } else
                stream.printf("Input index too large. Next available index: i%d.\n", input_count);

        } else if (command[0] == 'b' && parse_uint32(command+1, &idx)) {
            // Set definition for base station b<n>
            if (idx <= base_station_count && idx < max_num_base_stations) {
                BaseStationGeometry base_station;
                if (parse_base_station(input_string, &base_station, stream)) {
                    base_stations[idx] = base_station;
                    if (idx == base_station_count)
                        base_station_count = idx+1;
                    print_base_station(idx, base_stations[idx], stream);
                }
            } else
                stream.printf("Base station index too large. Next available index: b%d.\n", base_station_count);

        } else if (!strcmp(command, "reset")) {
            input_count = 0;
            base_station_count = 0;
            stream.printf("Reset successful.\n");

        } else if (!strcmp(command, "reload")) {
            read_from_eeprom();
            stream.printf("Loaded previous configuration from EEPROM.\n");

        } else if (!strcmp(command, "write")) {
            is_configured = true;
            write_to_eeprom();
            stream.printf("Write to EEPROM successful. Type 'continue' to start using it.\n");

        } else if (!strcmp(command, "continue")) {
            return;            

        } else {
            stream.printf("Unknown command '%s'. Valid commands: view, <module> <settings>, reset, reload, write, continue.\n", command);
        }
    }
}




