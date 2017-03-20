#pragma once
#include "output_serial.h"
#include <application.h>

class OutputNodeWifi : public OutputNode {
public:
    OutputNodeWifi(uint32_t idx, const OutputDef& def);

protected:
    virtual void start();
    virtual size_t write(const uint8_t *buffer, size_t size);
    virtual int read();
    static CreatorRegistrar creator_;

    uint16_t local_port_, remote_port_;
    IPAddress remote_ip_;
    UDP udp_stream_;
};
