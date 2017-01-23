/* see mavlink_helpers.h */
#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#define MAVLINK_COMM_NUM_BUFFERS 1
#define MAVLINK_SEND_UART_BYTES mavlink_send_uart_bytes

#include <mavlink_types.h>

// Communication settings of this system. Two random numbers.
mavlink_system_t mavlink_system = {
    .sysid = 155,
    .compid = 1,
};

void mavlink_send_uart_bytes(mavlink_channel_t chan, const uint8_t *chars, unsigned length);

#include <common/mavlink.h>

// TODO: Add messages in increasing msg_id order.
/*
#define MAVLINK_USE_MESSAGE_INFO
#undef MAVLINK_MESSAGE_INFO
#define MAVLINK_MESSAGE_INFO {MAVLINK_MESSAGE_INFO_HEARTBEAT, MAVLINK_MESSAGE_INFO_SYS_STATUS, MAVLINK_MESSAGE_INFO_SYSTEM_TIME, MAVLINK_MESSAGE_INFO_ATTITUDE, MAVLINK_MESSAGE_INFO_SERVO_OUTPUT_RAW, MAVLINK_MESSAGE_INFO_VFR_HUD, MAVLINK_MESSAGE_INFO_ATTITUDE_TARGET, MAVLINK_MESSAGE_INFO_HIGHRES_IMU, MAVLINK_MESSAGE_INFO_BATTERY_STATUS, MAVLINK_MESSAGE_INFO_EXTENDED_SYS_STATE}
#undef _MAVLINK_GET_INFO_H_
#include <mavlink_get_info.h>
*/
#include "main.h"

static HardwareSerial &MavlinkSerial = Serial1;

// Send multiple chars (uint8_t) over a comm channel
void mavlink_send_uart_bytes(mavlink_channel_t chan, const uint8_t *chars, unsigned length) {
    MavlinkSerial.write(chars, length);
}
/*
void print_mavlink_message(const mavlink_message_t &msg, Stream &dest) {
    dest.printf("> %u|%u: ", (unsigned)msg.sysid, (unsigned)msg.compid);

    const mavlink_message_info_t *msg_info = mavlink_get_message_info(&msg);
    if (msg_info && msg_info->msgid == msg.msgid) {
        dest.printf("%s (%u): ", msg_info->name, (unsigned)msg.msgid);
        for (unsigned i = 0; i < msg_info->num_fields; i++) {
            const mavlink_field_info_t& field = msg_info->fields[i];

            dest.print(field.name);
            dest.print("=");
            if (field.array_length == 0) {
                switch (field.type) {
                    case MAVLINK_TYPE_CHAR:    dest.print((unsigned)_MAV_RETURN_char(&msg, field.wire_offset)); break;
                    case MAVLINK_TYPE_UINT8_T: dest.print((unsigned)_MAV_RETURN_uint8_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_INT8_T:  dest.print((int)_MAV_RETURN_int8_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_UINT16_T:dest.print(_MAV_RETURN_uint16_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_INT16_T: dest.print(_MAV_RETURN_int16_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_UINT32_T:dest.print(_MAV_RETURN_uint32_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_INT32_T: dest.print(_MAV_RETURN_int32_t(&msg, field.wire_offset));  break;
                        //case MAVLINK_TYPE_UINT64_T:dest.print(_MAV_RETURN_uint64_t(&msg, field.wire_offset));  break;
                        //case MAVLINK_TYPE_INT64_T: dest.print(_MAV_RETURN_int64_t(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_FLOAT:   dest.print(_MAV_RETURN_float(&msg, field.wire_offset));  break;
                    case MAVLINK_TYPE_DOUBLE:  dest.print(_MAV_RETURN_double(&msg, field.wire_offset));  break;
                    default: dest.printf("<unknown type %d>", field.type);
                }
            } else {
                dest.printf("<array[%d]>", field.array_length); // TODO: values.
            }
            dest.print("; ");
        }
    } else {
        dest.printf("%u", (unsigned)msg.msgid);
    }
    dest.println();
}
*/
void handle_mavlink_message(const mavlink_message_t &message) {
//    switch (message.msgid) {
//        case MAVLINK_MSG_ID_ATT_POS_MOCAP: break;
//    }
    //print_mavlink_message(message, Serial);
}

void process_incoming_mavlink_messages() {
    mavlink_message_t message = {0};
    mavlink_status_t status;

    while (MavlinkSerial.available()) {
        uint8_t ch = (uint8_t)MavlinkSerial.read();
        uint8_t msg_received = mavlink_parse_char(MAVLINK_COMM_0, ch, &message, &status);

        // TODO: Compare status.packet_rx_drop_count with previous one.

        if (msg_received) {
            handle_mavlink_message(message);
        }
    }
}

static uint32_t last_ms = 0;

void send_mavlink_position(const float ned[3]) {
    uint64_t time_usec = 0; // Zero will be converted to the time of receive.
    float attitude_quaternion[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // Attitude is not used.

    mavlink_msg_att_pos_mocap_send(MAVLINK_COMM_0, time_usec, attitude_quaternion, ned[0], ned[1], ned[2]);

    uint32_t now_ms = millis();
    if (now_ms - last_ms > 50)
        Serial.printf("Late Mavlink message: %dms\n", now_ms - last_ms);
    last_ms = now_ms;
}
