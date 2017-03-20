#include "output_wifi.h"

OutputNodeWifi::OutputNodeWifi(uint32_t idx, const OutputDef& def)
    : OutputNode(idx, def)
    , udp_stream_{} {
    local_port_ = 33000;
    remote_port_ = 3300;
}

void OutputNodeWifi::start() {
    OutputNode::start();
    udp_stream_.begin(local_port_);
    uint32_t local_ip = WiFi.localIP().raw().ipv4;
    uint32_t subnet_mask = WiFi.subnetMask().raw().ipv4;
    uint32_t broadcast_ip = local_ip | ~subnet_mask;
    remote_ip_ = broadcast_ip;
}

size_t OutputNodeWifi::write(const uint8_t *buffer, size_t size) {
    size_t ret = udp_stream_.sendPacket(buffer, size, remote_ip_, remote_port_);
    if (ret < 0)
        Serial.printf("Error sending UDP packet: %d\n", ret);
    return ret;
}

int OutputNodeWifi::read() {
    if (!udp_stream_.available())
        udp_stream_.parsePacket();
    return udp_stream_.read();
}

OutputNode::CreatorRegistrar OutputNodeWifi::creator_([](uint32_t idx, const OutputDef& def) -> std::unique_ptr<OutputNode> {
    if (idx == 2)
        return std::make_unique<OutputNodeWifi>(idx, def);
    return nullptr;
});
