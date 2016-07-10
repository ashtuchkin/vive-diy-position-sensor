#include "main.h"

// Frame is 33 bytes long, see description here: https://github.com/nairol/LighthouseRedox/blob/master/docs/Base%20Station.md
// To decode float16, we can use ARM specific __fp16 type.

void initialize_decoders(input_data &d) {
    // Initialize all decoders to middle value
    for (int i = 0; i < num_big_pulses_in_cycle; i++) {
        for (int j = 0; j < num_cycle_phases; j++)
            d.decoders[i].bit_decoders[j] = {100 << 4, 10}; // Use 100 uS as middle value - it'll be corrected later.
    }
}

inline int decode_bit(bit_decoder &dec, int pulse_len) {
    if (pulse_len == 0)
        return -1;

    int delta_center = pulse_len - (dec.center_pulse_len >> 4);
    bool high = delta_center > 0;

    int assumed_center = pulse_len - (high ? dec.delta_width : -dec.delta_width);
    dec.center_pulse_len = (dec.center_pulse_len * 15 + (assumed_center << 4)) >> 4;

    // In future: auto-adjust delta_width as well

    if (abs(assumed_center - (dec.center_pulse_len >> 4)) < 5) {
        return int(high);
    } else {
        return -1; // Not accurate
    }
}

inline void decode_and_write_bit(decoder &dec, uint32_t phase_id, uint32_t pulse_len) {
    int bit = decode_bit(dec.bit_decoders[phase_id], int(pulse_len));

    data_frame &frame = dec.data_frames[dec.write_data_frames_idx];
    if (bit == -1) { // Not decoded -> reset frame.
        frame = {};
        return;
    }

    if (frame.waiting_for_one) {
        if (bit)
            frame.waiting_for_one = false;
        else
            frame = {};
        return;
    }

    // Searching for preamble (17 zeros).
    if (frame.preamble_len != 17) {
        if (bit) {
            frame.preamble_len = 0; // high bit -> reset preamble
        } else {
            frame.preamble_len++;
            if (frame.preamble_len == 17) {
                frame.waiting_for_one = true;
                frame.data_idx = -2; // 2 bytes for data len.
            }
        }
        return;
    }

    // Reading 8 bits into accumulator.
    frame.accumulator = (frame.accumulator << 1) | bit;
    frame.cur_bit_idx++;
    if (frame.cur_bit_idx != 8)
        return;

    if (frame.data_idx & 1)
        frame.waiting_for_one = true;

    // Process & write accumulator
    byte data_byte = (byte)frame.accumulator;
    frame.accumulator = 0;
    frame.cur_bit_idx = 0;
    if (frame.data_idx < 0)
        frame.data_len |= data_byte << ((2+frame.data_idx) * 8);
    else
        frame.data[frame.data_idx] = data_byte;
    frame.data_idx++;

    if (frame.data_idx == (frame.data_len|1) + 2) {
        // Received full frame - write it.
        INC_CONSTRAINED(dec.write_data_frames_idx, num_data_frames);
        dec.data_frames[dec.write_data_frames_idx] = {};
    }
}

void extract_data_from_cycle(input_data &d, uint32_t first_pulse_len, uint32_t second_pulse_len, uint32_t id) {
    if (id == 0)
        initialize_decoders(d);

    uint32_t phase_id = id % 4;

    decode_and_write_bit(d.decoders[0], phase_id, first_pulse_len);
    decode_and_write_bit(d.decoders[1], phase_id, second_pulse_len);
}
