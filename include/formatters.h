#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "messages.h"
#include "geometry.h"

enum class FormatterType {
    kAngles,
    kDataFrame,
    kPosition,
};
enum class FormatterSubtype {
    kPosText,
    kPosMavlink,
};

// Stored definition of a FormatterNode
struct FormatterDef {
    FormatterType formatter_type;
    FormatterSubtype formatter_subtype;
    uint32_t input_idx;  // For geometry formatters, it's the geometry builder idx.
    uint32_t output_idx;  // Which output node to send the data to.

    CoordSysType coord_sys_type;
    CoordSysDef coord_sys_params;

    void print_def(uint32_t idx, PrintStream &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream);
};


// Abstract base class for formatter nodes. They all get different inputs, but always produce data streams.
class FormatterNode 
    : public WorkerNode
    , public Producer<DataChunk> {
public:
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

protected:
    FormatterNode(uint32_t idx, const FormatterDef &def);

    uint32_t node_idx_;
    FormatterDef def_;
};

// Format sensor angles to a text form.
class SensorAnglesTextFormatter
    : public FormatterNode
    , public Consumer<SensorAnglesFrame> {
public:
    SensorAnglesTextFormatter(uint32_t idx, const FormatterDef &def) : FormatterNode(idx, def) {}
    virtual void consume(const SensorAnglesFrame& f);
};

// Base class for geometry formatters.
class GeometryFormatter 
    : public FormatterNode
    , public Consumer<ObjectPosition> {
public:
    static std::unique_ptr<GeometryFormatter> create(uint32_t idx, const FormatterDef &def);

protected:
    GeometryFormatter(uint32_t idx, const FormatterDef &def) : FormatterNode(idx, def) {}
};

// Format object geometry in a text form.
class GeometryTextFormatter : public GeometryFormatter {
public:
    GeometryTextFormatter(uint32_t idx, const FormatterDef &def) : GeometryFormatter(idx, def) {}
    virtual void consume(const ObjectPosition& f);
};


// Format object geometry in Mavlink format.
class GeometryMavlinkFormatter : public GeometryFormatter {
public:
    GeometryMavlinkFormatter(uint32_t idx, const FormatterDef &def);
    virtual void consume(const ObjectPosition& f);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream &stream);

private:
    bool position_valid(const ObjectPosition& g);
    void send_message(uint32_t msgid, const char *packet, Timestamp cur_time, uint8_t min_length, uint8_t length, uint8_t crc_extra);

    uint32_t current_tx_seq_;
    Timestamp last_message_timestamp_;  // LongTimestamp
    float last_pos_[3];
    bool debug_print_state_;
    uint32_t debug_late_messages_;
};

