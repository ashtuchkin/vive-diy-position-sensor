#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "common.h"

class OutputNode : public WorkerNode {

};

// Output sensor angles in a text form.
class SensorAnglesTextOutput
    : public OutputNode
    , public Consumer<SensorAnglesFrame> {
public:
    SensorAnglesTextOutput(Print &stream);
    virtual void consume(const SensorAnglesFrame& f);
    virtual bool debug_cmd(HashedWord *input_words);

private:
    Print *stream_;
    bool debug_disable_output_;
};


// Output object geometry in a text form.
class GeometryTextOutput
    : public OutputNode
    , public Consumer<ObjectGeometry> {
public:
    GeometryTextOutput(Print &stream, uint32_t object_idx);
    virtual void consume(const ObjectGeometry& f);
    virtual bool debug_cmd(HashedWord *input_words);

private:
    Print *stream_;
    uint32_t object_idx_;
    bool debug_disable_output_;
};


// Output geometry of an object in Mavlink format.
class MavlinkGeometryOutput
    : public OutputNode
    , public Consumer<ObjectGeometry> {
public:
    MavlinkGeometryOutput(Print &stream);
    virtual void consume(const ObjectGeometry& f);

    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);

    virtual ~MavlinkGeometryOutput();
private:
    bool position_valid(const ObjectGeometry& g);

    uint32_t stream_idx_;
    Timestamp last_message_timestamp_;
    float last_pos_[3];
    bool debug_print_state_;
    uint32_t debug_late_messages_;
};

