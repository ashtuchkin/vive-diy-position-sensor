#pragma once
#include <Arduino.h>
#undef __enable_irq
#undef __disable_irq
#include "utils.h"

const static int max_num_inputs = 8;
const static int max_num_base_stations = 2;
const static int num_comparators = 3;
const static int cycles_buffer_len = 128;
const static int pulses_buffer_len = 32;

const static int min_pulse_len = 2;       // uS
const static int min_big_pulse_len = 40;  // uS
const static int max_big_pulse_len = 300; // uS
const static int cycle_period = 8333;  // uS, total len of 1 cycle.
const static int second_big_pulse_delay = 400; // uS.
const static int angle_center_len = 4000; // uS

const static int num_big_pulses_in_cycle = 2;
const static int num_cycle_phases = 4;
const static int decoded_data_max_len = 50;
const static int num_data_frames = 8;

const static float max_position_jump = 0.05; // meters

enum InputType {
    kCMP = 0,  // Comparator
    kFTM = 1,  // Flexible Timer Module interrupts
    kPort = 2, // Digital input interrupt
    kMaxInputType
};

struct BitDecoder {
    float center_pulse_len; // uS
    int delta_width; // uS
};

struct DataFrame {
    int preamble_len;
    bool waiting_for_one;
    int data_len;
    int cur_bit_idx;
    int accumulator;
    int data_idx;
    byte data[decoded_data_max_len];
};

struct Decoder {
    BitDecoder bit_decoders[num_cycle_phases];

    DataFrame cur_frame;
    CircularBuffer<DataFrame, num_data_frames> data_frames;
};

struct Pulse {
    uint32_t start_time;
    uint32_t pulse_len;
};

struct Cycle {
    uint32_t start_time; // Microseconds of start of first pulse
    uint32_t phase_id;   // 0..3 - id of this cycle
    uint32_t first_pulse_len;
    uint32_t second_pulse_len;
    uint32_t laser_pulse_len;
    uint32_t laser_pulse_pos;
    uint32_t cmp_threshold;
};

struct InputData {
    CircularBuffer<Pulse, pulses_buffer_len> pulses;

    uint32_t num_pulses;      // total number of pulses received
    uint32_t small_pulses;    // number of small pulses (laser)
    uint32_t big_pulses;      // number of total big pulses
    uint32_t fake_big_pulses; // number of wrong big pulses

    uint32_t last_cycle_time; // uS timestamp of start of last successful cycle
    uint32_t last_cycle_id;   // 0..3
    Cycle cur_cycle;          // Currently constructed cycle

    CircularBuffer<Cycle, cycles_buffer_len> cycles;

    Decoder decoders[num_big_pulses_in_cycle];

    bool fix_acquired;
    uint32_t fix_cycle_offset;

    uint32_t angle_lens[num_cycle_phases];
    uint32_t angle_timestamps[num_cycle_phases];
    uint32_t angle_last_timestamp, angle_last_processed_timestamp;

    float last_ned[3];
};

// ==== Module functions ====

// Inputs
struct InputDefinition;
bool setupCmpInput(uint32_t input_idx, const InputDefinition &inp_def, char *error_message, bool validation_mode = false);
void resetCmpAfterValidation();
int getCmpThreshold(uint32_t input_idx);
int getCmpLevel(uint32_t input_idx);
void changeCmpThreshold(uint32_t input_idx, int delta);

// Main processing
void add_pulse(uint32_t input_idx, uint32_t start_time, uint32_t end_time);

// Data decoder
void extract_data_from_cycle(InputData &d, uint32_t first_pulse_len, uint32_t second_pulse_len, uint32_t id);

// Geometry
void calculate_3d_point(const uint32_t angle_lens[num_cycle_phases], float (*ned)[3], float *dist);
void convert_to_ned(const float pt[3], float (*ned)[3]);

// Ublox
void send_ublox_ned_position(Stream &stream, bool fix_valid, float *pos, float *vel); // all arguments NED, in mm and mm/s

// Mavlink
void process_incoming_mavlink_messages();
void send_mavlink_position(const float ned[3]);

