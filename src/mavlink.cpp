#include "outputs.h"

// This strange sequence of includes and definitions is due to how mavlink_helpers.h works :(
// TODO: This currently includes code to sign packets with SHA256, bloating the binary. Get rid of it.
#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#define MAVLINK_COMM_NUM_BUFFERS 4
#define MAVLINK_SEND_UART_BYTES mavlink_send_uart_bytes

#include <mavlink_types.h>

// Communication parameters of this system.
mavlink_system_t mavlink_system = {
    .sysid = 155,
    .compid = 1,
};

void mavlink_send_uart_bytes(mavlink_channel_t chan, const uint8_t *chars, unsigned length);

#include <common/mavlink.h>

// Insanity ends here. Back to normal.
#include <assert.h>
#include <Print.h>
#include <avr_emulation.h>
#include "primitives/vector.h"

// Static list of streams we will output to.
static Print *output_streams[MAVLINK_COMM_NUM_BUFFERS] = {};

// Send multiple chars (uint8_t) over a comm channel
void mavlink_send_uart_bytes(mavlink_channel_t chan, const uint8_t *chars, unsigned length) {
    output_streams[chan]->write(chars, length);
}

MavlinkGeometryOutput::MavlinkGeometryOutput(Print &stream)
    : stream_idx_(0)
    , last_message_timestamp_()
    , last_pos_{0, 0, 0}
    , debug_print_state_(false)
    , debug_late_messages_(0) {
    bool inserted = false;
    for (int i = 0; i < MAVLINK_COMM_NUM_BUFFERS; i++)
        if (!output_streams[i]) {
            stream_idx_ = i;
            output_streams[i] = &stream;
            inserted = true;
        }
    assert(inserted);
}

MavlinkGeometryOutput::~MavlinkGeometryOutput() {
    output_streams[stream_idx_] = nullptr;
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

    // TODO: Use our time here. Someday.
    // Zero will be converted to the time of receive.
    uint64_t time_usec = 0;
    mavlink_channel_t chan = static_cast<mavlink_channel_t>(stream_idx_);

    mavlink_msg_att_pos_mocap_send(chan, time_usec, g.q, g.xyz[0], g.xyz[1], g.xyz[2]);
}

bool MavlinkGeometryOutput::debug_cmd(HashedWord *input_words) {
    if (*input_words == "mavlink#"_hash && input_words->idx == stream_idx_) {
        input_words++;
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