#include <Arduino.h>
#include <arm_math.h>

const static int num_inputs = 1;
const static int cycles_buffer_len = 1024;

const static int min_pulse_len = 3;       // uS
const static int min_big_pulse_len = 40;  // uS
const static int max_big_pulse_len = 300; // uS
const static int cycle_period = 8333;  // uS, total len of 1 cycle.
const static int second_big_pulse_delay = 400; // uS.
const static int angle_center_len = 4000; // uS

const static int num_big_pulses_in_cycle = 2;
const static int num_cycle_phases = 4;
const static int decoded_data_max_len = 50;
const static int num_data_frames = 10;

const static float max_position_jump = 0.05; // meters

struct bit_decoder {
    int center_pulse_len; // uS
    int delta_width; // uS
};

struct data_frame {
    int preamble_len;
    bool waiting_for_one;
    int data_len;
    int cur_bit_idx;
    int accumulator;
    int data_idx;
    byte data[decoded_data_max_len];
};

struct decoder {
    bit_decoder bit_decoders[num_cycle_phases];

    int read_data_frames_idx, write_data_frames_idx;
    data_frame data_frames[num_data_frames];
};

struct cycle {
    uint32_t start_time; // Microseconds of start of first pulse
    uint32_t phase_id;   // 0..3 - id of this cycle
    uint32_t first_pulse_len;
    uint32_t second_pulse_len;
    uint32_t laser_pulse_len;
    uint32_t laser_pulse_pos;
    uint32_t dac_level;
};

struct input_data {
    uint32_t rise_time;  // uS, time of the start of current pulse
    uint32_t dac_level;  // level of dac, 0..63

    bool fix_acquired;
    uint32_t fix_cycle_offset;

    uint32_t crossings;       // number of interrupts
    uint32_t small_pulses;    // number of small pulses (laser)
    uint32_t big_pulses;      // number of total big pulses
    uint32_t fake_big_pulses; // number of wrong big pulses

    uint32_t last_cycle_time; // uS timestamp of start of last successful cycle
    uint32_t last_cycle_id;   // 0..3
    cycle cur_cycle;          // Currently constructed cycle

    int32_t cycles_write_idx, cycles_read_idx, cycles_read_geom_idx;
    cycle cycles[cycles_buffer_len];

    decoder decoders[num_big_pulses_in_cycle];

    uint32_t angle_lens[num_cycle_phases];
    uint32_t angle_timestamps[num_cycle_phases];
    uint32_t angle_last_timestamp, angle_last_processed_timestamp;

    float last_ned[3];
};

// ==== Module functions ====

extern input_data global_input_data[num_inputs];

// Data decoder
void extract_data_from_cycle(input_data &d, uint32_t first_pulse_len, uint32_t second_pulse_len, uint32_t id);

// Geometry
void calculate_3d_point(input_data& d, float (*ned)[3], float *dist);

// Ublox
void send_ublox_ned_position(Stream &stream, bool fix_valid, float *pos, float *vel); // all arguments NED, in mm and mm/s

// Mavlink
void process_incoming_mavlink_messages();
void send_mavlink_position(const float ned[3]);


// ==== Utils ====

#define INC_CONSTRAINED(val, size) val = ((val) < (size)-1 ? ((val)+1) : 0)


static inline void setCmpDacLevel(uint32_t level) { // level = 0..63
    // DAC Control: Enable; Voltage ref=3v3; Output select=0
    CMP0_DACCR = CMP_DACCR_DACEN | CMP_DACCR_VRSEL | CMP_DACCR_VOSEL(level);
}

static inline void changeCmdDacLevel(input_data &d, int delta) {
    d.dac_level = (uint32_t)max(1, min(63, (int)d.dac_level + delta));
    setCmpDacLevel(d.dac_level);
}

static inline int getCmpLevel() {  // Current signal level: 0 or 1
    return CMP0_SCR & CMP_SCR_COUT;
}
