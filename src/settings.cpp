// This file manages project configuration & persisting it to EEPROM.
#include "settings.h"
#include "primitives/string_utils.h"
#include "vive_sensors_pipeline.h"
#include "led_state.h"
#include "print_helpers.h"

constexpr uint32_t current_settings_version = 0xbabe0000 + sizeof(PersistentSettings);
constexpr uint32_t initial_eeprom_addr = 0;

static_assert(sizeof(PersistentSettings) < 1500, "PersistentSettings must fit into EEPROM with some leeway");
static_assert(std::is_trivially_copyable<PersistentSettings>(), "All definitions must be trivially copyable to be bitwise-stored");

/* Example settings
# Comments are prepended by '#'
reset  # Reset all settings to clean state
sensor0 pin 12 positive
base0 origin -1.528180 2.433750 -1.969390 matrix -0.841840 0.332160 -0.425400 -0.046900 0.740190 0.670760 0.537680 0.584630 -0.607540
base1 origin 1.718700 2.543170 0.725060 matrix 0.458350 -0.649590 0.606590 0.028970 0.693060 0.720300 -0.888300 -0.312580 0.336480
object0 sensor0
stream0 position object0 > usb_serial
stream1 angles > usb_serial
serial1 57600
stream2 mavlink object0 ned 110 > serial1
*/

PersistentSettings settings;

PersistentSettings::PersistentSettings() {
    reset();
    read_from_eeprom();
}

bool PersistentSettings::read_from_eeprom() {
    uint32_t eeprom_addr = initial_eeprom_addr, version;
    eeprom_read(eeprom_addr, &version, sizeof(version)); eeprom_addr += sizeof(version);
    if (version == current_settings_version) {
        // Normal initialization: Just copy block of data.
        eeprom_read(eeprom_addr, this, sizeof(*this));
        return true;
    } 
    // Unknown version.
    return false;
}

void PersistentSettings::write_to_eeprom() {
    uint32_t version = current_settings_version;
    uint32_t eeprom_addr = initial_eeprom_addr;
    eeprom_write(eeprom_addr, &version, sizeof(version));  eeprom_addr += sizeof(version);
    eeprom_write(eeprom_addr, this, sizeof(*this));
}

void PersistentSettings::restart_in_configuration_mode() {
    is_configured_ = false;
    write_to_eeprom();
    restart_system();
}

// Initialize settings.
void PersistentSettings::reset() {
    memset(this, 0, sizeof(*this));

    // Defaults.
    outputs_.set_size(num_outputs);
    outputs_[0].active = true;
}

bool PersistentSettings::validate_setup(PrintStream &error_stream) {
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
void PersistentSettings::set_value(Vector<T, arr_len> &arr, uint32_t idx, HashedWord *input_words, PrintStream &stream) {
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
                stream.printf("Updated: ");
                arr[idx].print_def(idx, stream);
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

bool PersistentSettings::process_command(char *input_cmd, PrintStream &stream) {
    HashedWord *input_words = hash_words(input_cmd);
    if (*input_words && input_words->word[0] != '#') { // Do nothing on comments and empty lines.
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
            is_configured_ = true;
            return false;

        default:
            stream.printf("Unknown command '%s'. Valid commands: view, <module> <settings>, reset, reload, write, validate, continue.\n", (input_words-1)->word);
        }
    }

    // Print prompt for the next command.
    stream.printf("config> ");
    return true;
}


// == Configuration pipeline ==================================================

class SettingsReaderWriterNode 
    : public WorkerNode
    , public DataChunkLineSplitter
    , public Producer<DataChunk> {
public:
    SettingsReaderWriterNode(PersistentSettings *settings, Pipeline *pipeline)
            : settings_(settings)
            , pipeline_(pipeline) {
        set_led_state(LedState::kConfigMode);
    }

private:
    virtual void consume_line(char *line, Timestamp time) {
        DataChunkPrintStream stream(this, time);
        if (!settings_->process_command(line, stream))
            pipeline_->stop();
    }

    virtual void do_work(Timestamp cur_time) {
        update_led_pattern(cur_time);
    }

    PersistentSettings *settings_;
    Pipeline *pipeline_;
};

std::unique_ptr<Pipeline> PersistentSettings::create_configuration_pipeline(uint32_t stream_idx) {
    auto pipeline = std::make_unique<Pipeline>();

    auto reader = pipeline->add_back(std::make_unique<SettingsReaderWriterNode>(this, pipeline.get()));
    auto output_node = pipeline->add_back(OutputNode::create(stream_idx, OutputDef{}));
    reader->pipe(output_node);
    output_node->pipe(reader);

    return pipeline;
}
