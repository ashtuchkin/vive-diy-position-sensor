#include "output_serial.h"
#include <application.h>


// ======  OutputNodeStream  ==================================================

OutputNodeStream::OutputNodeStream(uint32_t idx, const OutputDef& def, Stream &stream)
    : OutputNode(idx, def)
    , stream_(stream) {
}

size_t OutputNodeStream::write(const uint8_t *buffer, size_t size) {
    return stream_.write(buffer, size);
}

int OutputNodeStream::read() {
    return stream_.read();
}

// ======  UsbSerialOutputNode  ===============================================

UsbSerialOutputNode::UsbSerialOutputNode(uint32_t idx, const OutputDef& def) 
    : OutputNodeStream(idx, def, Serial) {
    assert(idx == 0);
}

OutputNode::CreatorRegistrar UsbSerialOutputNode::creator_([](uint32_t idx, const OutputDef& def) -> std::unique_ptr<OutputNode> {
    if (idx == 0)
        return std::make_unique<UsbSerialOutputNode>(idx, def);
    return nullptr;
});


// ======  HardwareSerialOutputNode  ==========================================

static HardwareSerial *hardware_serials[num_outputs] = {nullptr, &Serial1, nullptr, nullptr};

HardwareSerialOutputNode::HardwareSerialOutputNode(uint32_t idx, const OutputDef& def) 
    : OutputNodeStream(idx, def, *hardware_serials[idx]) {
    assert(idx < num_outputs && hardware_serials[idx]);
    // TODO: check bitrate and serial format are valid.
}

void HardwareSerialOutputNode::start() {
    reinterpret_cast<HardwareSerial *>(&stream_)->begin(def_.bitrate);
}

OutputNode::CreatorRegistrar HardwareSerialOutputNode::creator_([](uint32_t idx, const OutputDef& def) -> std::unique_ptr<OutputNode> {
    if (idx < num_outputs && hardware_serials[idx])
        return std::make_unique<HardwareSerialOutputNode>(idx, def);
    return nullptr;
});
