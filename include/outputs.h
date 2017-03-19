// This file defines OutputNode-s. These are dumb IO Nodes reading and writing DataChunk-s to/from different
// hardware streams.
#pragma once
#include "primitives/workers.h"
#include "primitives/producer_consumer.h"
#include "primitives/static_registration.h"
#include "messages.h"

// Currently supported: usb serial + 3x hardware serials.
constexpr int num_outputs = 4;

struct OutputDef {
    bool active;
    uint32_t bitrate;

    void print_def(uint32_t idx, PrintStream &stream);
    bool parse_def(uint32_t idx, HashedWord *input_words, PrintStream &err_stream);
};

class OutputNode
    : public WorkerNode
    , public Consumer<DataChunk>
    , public Consumer<OutputCommand>
    , public Producer<DataChunk> {
public:
    static std::unique_ptr<OutputNode> create(uint32_t idx, const OutputDef& def);
    typedef StaticRegistrar<decltype(create)*> CreatorRegistrar;

    // Common methods that do i/o with the stream_ object.
    virtual void consume(const DataChunk &chunk);
    virtual void consume(const OutputCommand& cmd);
    virtual void do_work(Timestamp cur_time);

protected:
    OutputNode(uint32_t idx, const OutputDef& def);

    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual int read() = 0;

    uint32_t node_idx_;
    OutputDef def_;
    DataChunk chunk_;
    bool exclusive_mode_;
    uint32_t exclusive_stream_idx_;
};

