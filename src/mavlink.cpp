#include "outputs.h"
#include <assert.h>
#include <common/mavlink.h>
#include <Print.h>
#include <avr_emulation.h>

// Communication parameters of this system.
mavlink_system_t mavlink_system = {
    .sysid = 155,
    .compid = 1,
};

MavlinkGeometryOutput::MavlinkGeometryOutput(Print &stream)
    : stream_(stream)
    , current_tx_seq_(0)
    , last_message_timestamp_()
    , last_pos_{0, 0, 0}
    , debug_print_state_(false)
    , debug_late_messages_(0) {

}

bool MavlinkGeometryOutput::position_valid(const ObjectGeometry& g) {
    // Filter out outliers.
    constexpr float max_position_jump = 0.05; // meters
    bool is_valid = false;
    if ((g.time - last_message_timestamp_) > TimeDelta(500, msec) ||
        (fabsf(g.xyz[0] - last_pos_[0]) < max_position_jump &&
         fabsf(g.xyz[1] - last_pos_[1]) < max_position_jump &&
         fabsf(g.xyz[2] - last_pos_[2]) < max_position_jump)) {
        is_valid = true;
    }
    if ((g.time - last_message_timestamp_) > TimeDelta(50, ms))
        debug_late_messages_++;

    last_message_timestamp_ = g.time;
    for (int i = 0; i < 3; i++)
        last_pos_[i] = g.xyz[i];
    return is_valid;
}

void MavlinkGeometryOutput::consume(const ObjectGeometry& g) {
    // Send the data to given stream. 
    // NOTE: The sending is synchronous, i.e. it won't return before the sending completes.
    // We might want to introduce a buffering here.

    // First, filter out outliers.
    if (!position_valid(g))
        return;

    mavlink_att_pos_mocap_t packet;

    // TODO: Use our time here. Someday.
    // Zero will be converted to the time of receive.
    packet.time_usec = 0;
    packet.x = g.xyz[0];
    packet.y = g.xyz[1];
    packet.z = g.xyz[2];
    mav_array_memcpy(packet.q, g.q, sizeof(float)*4);
    send_message(MAVLINK_MSG_ID_ATT_POS_MOCAP, (const char *)&packet, 
                 MAVLINK_MSG_ID_ATT_POS_MOCAP_MIN_LEN, 
                 MAVLINK_MSG_ID_ATT_POS_MOCAP_LEN, 
                 MAVLINK_MSG_ID_ATT_POS_MOCAP_CRC);
}

// Derived from _mav_finalize_message_chan_send
// We reimplement it here to avoid depending on channel machinery, SHA256 signatures and too large stack usage.
void MavlinkGeometryOutput::send_message(uint32_t msgid, const char *packet, uint8_t min_length, uint8_t length, uint8_t crc_extra)
{
	uint16_t checksum;
	uint8_t buf[MAVLINK_NUM_HEADER_BYTES];
	uint8_t ck[2];
    uint8_t header_len = MAVLINK_CORE_HEADER_LEN;
	bool mavlink1 = false;

    if (mavlink1) {
        length = min_length;
        assert(msgid <= 255); // can't send 16 bit messages
        header_len = MAVLINK_CORE_HEADER_MAVLINK1_LEN;
        buf[0] = MAVLINK_STX_MAVLINK1;
        buf[1] = length;
        buf[2] = current_tx_seq_++;
        buf[3] = mavlink_system.sysid;
        buf[4] = mavlink_system.compid;
        buf[5] = msgid & 0xFF;
    } else {
	    uint8_t incompat_flags = 0;
        length = _mav_trim_payload(packet, length);
        buf[0] = MAVLINK_STX;
        buf[1] = length;
        buf[2] = incompat_flags;
        buf[3] = 0; // compat_flags
        buf[4] = current_tx_seq_++;
        buf[5] = mavlink_system.sysid;
        buf[6] = mavlink_system.compid;
        buf[7] = msgid & 0xFF;
        buf[8] = (msgid >> 8) & 0xFF;
        buf[9] = (msgid >> 16) & 0xFF;
    }
	checksum = crc_calculate((const uint8_t*)&buf[1], header_len);
	crc_accumulate_buffer(&checksum, packet, length);
	crc_accumulate(crc_extra, &checksum);
	ck[0] = (uint8_t)(checksum & 0xFF);
	ck[1] = (uint8_t)(checksum >> 8);

	stream_.write((const char *)buf, header_len+1);
	stream_.write(packet, length);
	stream_.write((const char *)ck, 2);
}

bool MavlinkGeometryOutput::debug_cmd(HashedWord *input_words) {
    if (*input_words++ == "mavlink"_hash) {
        switch (*input_words++) {
            case "show"_hash: debug_print_state_ = true; return true;
            case "off"_hash: debug_print_state_ = false; return true;
        }
    }
    return false;
}

void MavlinkGeometryOutput::debug_print(Print &stream) {
    if (debug_print_state_) {
        if (debug_late_messages_ > 0)
            stream.printf("Late Mavlink messages: %d\n", debug_late_messages_);
        debug_late_messages_ = 0;
    }
}