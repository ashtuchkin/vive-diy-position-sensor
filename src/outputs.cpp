#include "outputs.h"
#include <Arduino.h>

static HardwareSerial *hardware_serials[num_outputs] = {nullptr, &Serial1, &Serial2, &Serial3};

OutputNode::OutputNode(uint32_t idx, const OutputDef& def, Stream &stream)
    : node_idx_(idx)
    , def_(def)
    , stream_(stream)
    , chunk_{}
    , exclusive_mode_(false)
    , exclusive_stream_idx_(0) {
}

std::unique_ptr<OutputNode> OutputNode::create(uint32_t idx, const OutputDef& def) {
    if (idx == 0)
        return std::make_unique<UsbSerialOutputNode>(idx, def);
    else if (idx < num_outputs)
        return std::make_unique<HardwareSerialOutputNode>(idx, def);
    else
        throw_printf("Invalid output index: %d", idx);
}

void OutputNode::consume(const DataChunk &chunk) {
    if (exclusive_mode_ && exclusive_stream_idx_ != chunk.stream_idx)
        return;
    
    // NOTE: This will wait until we can fit data into write buffer. Can cause a delay.
    stream_.write(&chunk.data[0], chunk.data.size());
}

void OutputNode::consume(const OutputCommand& cmd) {
    switch (cmd.type) {
        case OutputCommandType::kMakeExclusive: exclusive_mode_ = true; exclusive_stream_idx_ = cmd.stream_idx; break;
        case OutputCommandType::kMakeNonExclusive: exclusive_mode_ = false; break;
    }
}

void OutputNode::do_work(Timestamp cur_time) {
    // Accumulate bytes read from the stream_ in the chunk_.
    while (!chunk_.data.full()) {
        int c = stream_.read();
        if (c < 0)
            break;
        chunk_.time = cur_time;  // Store the time of the last byte read.
        chunk_.data.push(c);
    }

    // Send chunk if data is full or time from last byte is over given threshold.
    constexpr TimeDelta max_time_from_last_byte(1, msec);
    if (chunk_.data.full() || (!chunk_.data.empty() && cur_time - chunk_.time > max_time_from_last_byte)) {
        chunk_.last_chunk = !chunk_.data.full();
        chunk_.stream_idx = node_idx_;
        produce(chunk_);
        chunk_.data.clear();
    }
}


// ======  UsbSerialOutputNode  ===============================================

UsbSerialOutputNode::UsbSerialOutputNode(uint32_t idx, const OutputDef& def) 
    : OutputNode(idx, def, Serial) {
    assert(idx == 0);
}


// ======  HardwareSerialOutputNode  ==========================================

HardwareSerialOutputNode::HardwareSerialOutputNode(uint32_t idx, const OutputDef& def) 
    : OutputNode(idx, def, *hardware_serials[idx]) {
    assert(idx > 0 && idx < num_outputs);
    // TODO: check bitrate and serial format are valid.
}

void HardwareSerialOutputNode::start() {
    reinterpret_cast<HardwareSerial *>(&stream_)->begin(def_.bitrate);
}


// ======  OutputDef I/O  =====================================================

void OutputDef::print_def(uint32_t idx, Print &stream) {
    if (idx == 0 && !active) {
        stream.printf("usb_serial off\n");
    } else if (idx != 0 && active) {
        stream.printf("serial%d %d\n", idx, bitrate);
    }
}

bool OutputDef::parse_def(uint32_t idx, HashedWord *input_words, Print &err_stream) {
    if (idx == 0 || idx == (uint32_t)-1) {
        // usb_serial: do nothing unless turned off.
        active = (*input_words != "off"_hash);
        return true;
    } else if (idx < num_outputs) {
        // hardware serial
        active = true;
        if (!input_words->as_uint32(&bitrate) || !(300 <= bitrate && bitrate <= 115200)) {
            err_stream.printf("Invalid bitrate: %d. Needs to be in 300-115200 range.\n", bitrate);
            return false;
        }
        return true;            
    }
    return false;
}
