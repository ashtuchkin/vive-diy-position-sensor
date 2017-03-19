// NOTE: This file is not used at the moment.

// See https://github.com/iNavFlight/inav/blob/master/src/main/io/gps_ublox.c
// UBLOX: https://www.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_%28GPS.G6-SW-10018%29_Public.pdf

// Binary protocol.
// First receive a bunch of data. Ignore.
// Then, 5Hz send POSLLH (required), VELNED (required), SOL - sats.

// Binary protocol:
// 0xB5, 0x62, <class>, <msg>, <16 bit length (le)>, <payload>, <checksum a>, <checksum b>
// checksum is from class to end of payload.

static const int MAX_UBLOX_PAYLOAD_SIZE = 100;  // 100 bytes should be enough for all used messages
static const float DISTANCE_BETWEEN_TWO_LONGITUDE_POINTS_AT_EQUATOR = 11.1320f;  // in mm

typedef struct {
    uint8_t preamble1;
    uint8_t preamble2;
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
} ubx_header;

typedef struct {
    ubx_header header;
    uint8_t payload[MAX_UBLOX_PAYLOAD_SIZE+2];
} ubx_packet;

typedef struct {                   // GPS msToW
    uint32_t time;
    int32_t time_nsec;
    int16_t week;
    uint8_t fix_type;              // see enum ubs_nav_fix_type   <--
    uint8_t fix_status;            // see ubx_nav_status_bits   <--
    int32_t ecef_x;
    int32_t ecef_y;
    int32_t ecef_z;
    uint32_t position_accuracy_3d;
    int32_t ecef_x_velocity;
    int32_t ecef_y_velocity;
    int32_t ecef_z_velocity;
    uint32_t speed_accuracy;
    uint16_t position_DOP;           // cm  dilution of position <--
    uint8_t res;
    uint8_t satellites;              // number of sats   <--
    uint32_t res2;
} ubx_nav_solution;

typedef struct {
    uint32_t time;                // GPS ms ToW
    int32_t longitude;            // degrees * 1e7   <--
    int32_t latitude;             // degrees * 1e7   <--
    int32_t altitude_ellipsoid;   // mm
    int32_t altitude_msl;         // mm  sea level <--
    uint32_t horizontal_accuracy; // mm  one sigma estimated position error <--
    uint32_t vertical_accuracy;   // mm  one sigma estimated position error <--
} ubx_nav_posllh;

typedef struct {
    uint32_t time;              // GPS msToW
    int32_t ned_north;          // cm/s  <--
    int32_t ned_east;           // cm/s  <--
    int32_t ned_down;           // cm/s  <--
    uint32_t speed_3d;
    uint32_t speed_2d;          // cm/s   <--
    int32_t heading_2d;         // deg * 1e5   <--
    uint32_t speed_accuracy;
    uint32_t heading_accuracy;
} ubx_nav_velned;

enum ubx_protocol_bytes {
    PREAMBLE1 = 0xb5,
    PREAMBLE2 = 0x62,
    CLASS_NAV = 0x01,
    CLASS_ACK = 0x05,
    CLASS_CFG = 0x06,
    CLASS_MON = 0x0A,
    MSG_ACK_NACK = 0x00,
    MSG_ACK_ACK = 0x01,
    MSG_NAV_POSLLH = 0x2,
    MSG_NAV_STATUS = 0x3,
    MSG_NAV_SOL = 0x6,
    MSG_NAV_PVT = 0x7,
    MSG_NAV_VELNED = 0x12,
    MSG_NAV_SVINFO = 0x30,
    MSG_CFG_PRT = 0x00,
    MSG_CFG_RATE = 0x08,
    MSG_CFG_SET_RATE = 0x01,
    MSG_CFG_NAV_SETTINGS = 0x24
};

enum ubs_nav_fix_type {
    FIX_NONE = 0,
    FIX_DEAD_RECKONING = 1,
    FIX_2D = 2,
    FIX_3D = 3,
    FIX_GPS_DEAD_RECKONING = 4,
    FIX_TIME = 5
};

enum ubx_nav_status_bits {
    NAV_STATUS_FIX_VALID = 1
};

void send_ublox_packet(PrintStream &stream, uint8_t msg_class, uint8_t msg_id, const void *payload, uint8_t payload_len) {
    ubx_packet packet = {{PREAMBLE1, PREAMBLE2, msg_class, msg_id, payload_len}, {}};

    memcpy(packet.payload, payload, payload_len);

    // Calculate checksum
    uint8_t ck_a = 0, ck_b = 0;
    for (int i = -4; i < payload_len; i++) // -4 is because we should start with msg_class in header.
        ck_b += (ck_a += packet.payload[i]);

    packet.payload[payload_len] = ck_a;
    packet.payload[payload_len+1] = ck_b;

    // Write the whole packet.
    stream.write((const uint8_t *)&packet, sizeof(packet.header) + payload_len + 2);
}

// all arguments in mm and mm/s
void send_ublox_ned_position(PrintStream &stream, bool fix_valid, float *pos, float *vel) {
    const static int origin_lat = 1000000; // .1 degree
    const static int origin_lon = 1000000; // .1 degree
    const static int accuracy = 10; // mm

    ubx_nav_solution sol = {};
    if (fix_valid) {
        sol.fix_type = FIX_3D;
        sol.fix_status = NAV_STATUS_FIX_VALID;
        sol.position_DOP = accuracy / 10; // cm
        sol.satellites = 10;
    } else {
        sol.fix_type = FIX_NONE;
    }
    send_ublox_packet(stream, CLASS_NAV, MSG_NAV_SOL, &sol, sizeof(sol));

    if (fix_valid) {
        ubx_nav_posllh posllh = {};
        posllh.longitude = origin_lon + (int)(pos[1] / DISTANCE_BETWEEN_TWO_LONGITUDE_POINTS_AT_EQUATOR); // deg * 1e7
        posllh.latitude  = origin_lat + (int)(pos[0] / DISTANCE_BETWEEN_TWO_LONGITUDE_POINTS_AT_EQUATOR); // deg * 1e7
        posllh.altitude_msl = -(int)pos[3];     // mm
        posllh.horizontal_accuracy = accuracy;  // mm
        posllh.vertical_accuracy = accuracy;    // mm
        send_ublox_packet(stream, CLASS_NAV, MSG_NAV_POSLLH, &posllh, sizeof(posllh));

        ubx_nav_velned velned = {};
        velned.ned_north = (int)(vel[0] * .1); // cm/s
        velned.ned_east = (int)(vel[1] * .1);  // cm/s
        velned.ned_down = (int)(vel[2] * .1);  // cm/s

        velned.speed_2d = 0;   // cm/s         TODO. Not really used.
        velned.heading_2d = 0; // deg * 1e-5   TODO. Not really used.
        send_ublox_packet(stream, CLASS_NAV, MSG_NAV_VELNED, &velned, sizeof(velned));
    }
}

