#pragma once
#include "outputs.h"

class Stream;

class OutputNodeStream : public OutputNode {
public:
    OutputNodeStream(uint32_t idx, const OutputDef& def, Stream &stream);

protected:
    virtual size_t write(const uint8_t *buffer, size_t size);
    virtual int read();

    Stream &stream_;
};

class UsbSerialOutputNode : public OutputNodeStream {
public:
    UsbSerialOutputNode(uint32_t idx, const OutputDef& def);
    static CreatorRegistrar creator_;
};

class HardwareSerialOutputNode : public OutputNodeStream {
public:
    HardwareSerialOutputNode(uint32_t idx, const OutputDef& def);
    virtual void start();
    static CreatorRegistrar creator_;
};
