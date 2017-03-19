#include "formatters.h"
#include <stdio.h>
#include <stdarg.h>
#include "print_helpers.h"
#include "message_logging.h"

// ======  FormatterNode  =====================================================
FormatterNode::FormatterNode(uint32_t idx, const FormatterDef &def)
    : node_idx_(idx)
    , def_(def) {
}

bool FormatterNode::debug_cmd(HashedWord *input_words) {
    if (*input_words == "stream#"_hash && input_words->idx == node_idx_) {
        input_words++;
        return producer_debug_cmd(this, input_words, "DataChunk", node_idx_);
    }
    return false;
}
void FormatterNode::debug_print(PrintStream &stream) {
    producer_debug_print(this, stream);
}

// ======  SensorAnglesTextFormatter  =========================================
void SensorAnglesTextFormatter::consume(const SensorAnglesFrame& f) {
    uint32_t time = f.time.get_value(msec);

    // Print each sensor on its own line.
    for (uint32_t i = 0; i < f.sensors.size(); i++) {
        DataChunkPrintStream printer(this, f.time, node_idx_);
        const SensorAngles &angles = f.sensors[i];
        printer.printf("ANG%d\t%u\t%d", i, time, f.fix_level);
        for (uint32_t j = 0; j < num_cycle_phases; j++) {
            printer.printf("\t");
            if (f.fix_level == FixLevel::kCycleSynced && angles.updated_cycles[j] == f.cycle_idx - f.phase_id + j)
                printer.printf("%.4f", angles.angles[j]);
        }
        printer.printf("\n");
    }
}

// ======  GeometryFormatter  =================================================
std::unique_ptr<GeometryFormatter> GeometryFormatter::create(uint32_t idx, const FormatterDef &def) {
    switch (def.formatter_subtype) {
        case FormatterSubtype::kPosText:    return std::make_unique<GeometryTextFormatter>(idx, def);
        case FormatterSubtype::kPosMavlink: return std::make_unique<GeometryMavlinkFormatter>(idx, def);
        default: throw_printf("Unknown geometry formatter subtype: %d", def.formatter_subtype);
    }
}

// ======  GeometryTextFormatter  =============================================
void GeometryTextFormatter::consume(const ObjectPosition& f) {
    DataChunkPrintStream printer(this, f.time, node_idx_);
    printer.printf("OBJ%d\t%u\t%d", f.object_idx, f.time.get_value(msec), f.fix_level);
    if (f.fix_level >= FixLevel::kStaleFix) {
        printer.printf("\t%.4f\t%.4f\t%.4f\t%.4f", f.pos[0], f.pos[1], f.pos[2], f.pos_delta);
        if (f.q[0] != 1.0f) {  // Output quaternion if available.
            printer.printf("\t%.4f\t%.4f\t%.4f\t%.4f", f.q[0], f.q[1], f.q[2], f.q[3]);
        }
    }
    printer.printf("\n");
}

// ======  FormatterDef I/O  =====================================================
// Format: stream<idx> <type> <settings> > <output>
// stream0 mavlink object0 ned 110 > serial1
// stream1 angles > usb_serial
// stream2 position object0 > usb_serial

HashedWord formatter_types[] = {
    {"angles",    "angles"_hash,    (int)FormatterType::kAngles    << 16 },
    {"dataframe", "dataframe"_hash, (int)FormatterType::kDataFrame << 16 },
    {"position",  "position"_hash,  (int)FormatterType::kPosition  << 16 | (int)FormatterSubtype::kPosText},
    {"mavlink",   "mavlink"_hash,   (int)FormatterType::kPosition  << 16 | (int)FormatterSubtype::kPosMavlink},
};


void FormatterDef::print_def(uint32_t idx, PrintStream &stream) {
    stream.printf("stream%d ", idx);

    // Print type & subtype.
    uint32_t packed_type = (int)formatter_type << 16 | (int)formatter_subtype;
    for (uint32_t i = 0; i < sizeof(formatter_types) / sizeof(formatter_types[0]); i++)
        if (packed_type == formatter_types[i].idx) {
            stream.printf("%s ", formatter_types[i].word);
        }

    // Print settings for the type.
    switch (formatter_type) {
        case FormatterType::kAngles: break;
        case FormatterType::kDataFrame: break;
        case FormatterType::kPosition: {
            stream.printf("object%d ", input_idx);
            switch (coord_sys_type) {
                case CoordSysType::kDefault: break; // Nothing
                case CoordSysType::kNED: {
                    stream.printf("ned %.1f ", coord_sys_params.ned.north_angle);
                    break;
                }
            }
            break;
        }
    }

    // Print output idx.
    if (output_idx == 0)
        stream.printf("> usb_serial\n");
    else
        stream.printf("> serial%d\n", output_idx);
}

bool FormatterDef::parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream) {
    bool type_found = false;
    for (uint32_t i = 0; i < sizeof(formatter_types) / sizeof(formatter_types[0]); i++)
        if (*input_words == formatter_types[i]) {
            uint32_t packed_type = formatter_types[i].idx;
            formatter_type = (FormatterType)(packed_type >> 16);
            formatter_subtype = (FormatterSubtype)(packed_type & 0xFFFF);
            type_found = true;
        }
    input_words++;    
    if (!type_found) {
        err_stream.printf("Unknown stream type: '%s'\n", input_words->word);
        return false;
    }

    // Parse settings for type/subtype
    switch (formatter_type) {
        case FormatterType::kAngles: break;
        case FormatterType::kDataFrame: break;
        case FormatterType::kPosition: {
            if (*input_words != "object#"_hash) {
                err_stream.printf("Need object for position stream type.\n");
                return false;
            }
            input_idx = input_words->idx;
            input_words++;

            coord_sys_type = CoordSysType::kDefault;
            if (*input_words == "ned"_hash) {
                coord_sys_type = CoordSysType::kNED;
                input_words++;
                if (!input_words->as_float(&coord_sys_params.ned.north_angle)) {
                    err_stream.printf("Expected north angle after 'ned' keyword.\n");
                    return false;
                }
                input_words++;
            }
            break;
        }
    }

    if (*input_words++ != ">"_hash) {
        err_stream.printf("Expected '>' symbol\n"); return false;
    }

    if (*input_words == "serial#"_hash || *input_words == "usb_serial"_hash) {
        output_idx = *input_words == "usb_serial"_hash ? 0 : input_words->idx;
        return true;
    } else {
        err_stream.printf("Expected 'serial<num>' or 'usb_serial' as the output of the stream.\n");
        return false;
    }
    return false;
}
