#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "common.h"

// usb serial + 3x hardware serials.
constexpr int num_outputs = 4;

struct OutputDef {
    bool active;
    uint32_t bitrate;

    void print_def(uint32_t idx, Print &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, Print &err_stream);
};

class Stream;

class OutputNode
    : public WorkerNode
    , public Consumer<DataChunk>
    , public Consumer<OutputCommand>
    , public Producer<DataChunk> {
public:
    static std::unique_ptr<OutputNode> create(uint32_t idx, const OutputDef& def);

    // Common methods that do i/o with the stream_ object.
    virtual void consume(const DataChunk &chunk);
    virtual void consume(const OutputCommand& cmd);
    virtual void do_work(Timestamp cur_time);

protected:
    OutputNode(uint32_t idx, const OutputDef& def, Stream &stream);
    uint32_t node_idx_;
    OutputDef def_;
    Stream &stream_;
    DataChunk chunk_;
    bool exclusive_mode_;
    uint32_t exclusive_stream_idx_;
};


class UsbSerialOutputNode : public OutputNode {
public:
    UsbSerialOutputNode(uint32_t idx, const OutputDef& def);
};

class HardwareSerialOutputNode : public OutputNode {
public:
    HardwareSerialOutputNode(uint32_t idx, const OutputDef& def);
    virtual void start();
};
