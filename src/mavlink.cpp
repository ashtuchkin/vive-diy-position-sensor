#include "outputs.h"

// This strange sequence of includes and definitions is due to how mavlink_helpers.h works :(
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
#include <Arduino.h>  // Only for Print class.
#include "primitives/vector.h"

// Static list of streams we will output to.
Vector<Print *, MAVLINK_COMM_NUM_BUFFERS> output_streams;

// Send multiple chars (uint8_t) over a comm channel
void mavlink_send_uart_bytes(mavlink_channel_t chan, const uint8_t *chars, unsigned length) {
    output_streams[chan]->write(chars, length);
}

MavlinkGeometryOutput::MavlinkGeometryOutput(Print &stream) {
    if (!output_streams.full()) {
        stream_idx_ = output_streams.size();
        output_streams.push(&stream);
    } else {
        // Assert: full.
    }
}

void MavlinkGeometryOutput::reset_all() {
    output_streams.clear();
}

void MavlinkGeometryOutput::consume(const ObjectGeometry& g) {
    // Send the data to given stream. 
    // NOTE: The sending is synchronous, i.e. it won't return before the sending completes.
    // We might want to introduce a buffering here.

    // TODO: Use our time here.
    uint64_t time_usec = 0; // Zero will be converted to the time of receive.
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
        uint32_t now_ms = millis();
        if (now_ms - debug_last_ms_ > 50)
            Serial.printf("Late Mavlink message: %dms\n", now_ms - debug_last_ms_);
        debug_last_ms_ = now_ms;
    }
}