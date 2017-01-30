#include "main.h"
#include "settings.h"
#include "utils.h"

// Input-specific data
InputData global_input_data[max_num_inputs] = {};

// Debugging state.
uint32_t active_input_idx = 0;  // Active input for which we print values.
bool printPosition = true;

// Process debugging input/output.
void debug_io(Stream &stream) {
    // State of debugging. These values are kept between calls to debug_io().
    static bool printCountDelta = false, printCycles = false, printFrames = false, printDecoders = false;
    static int prevCycleId = -1;

    if (settings.input_count == 0)
        stream.printf("Error: No inputs configured. Please enter '!' to go to configuration mode and setup at least one input.\n");

    // Process input. All commands are single characters, usually followed by 'enter' key.
    while (stream.available()) {
        char c = stream.read();
        if ('0' <= c && c <= '9') { // Switch active input.
            uint32_t idx = c - '0';
            if (idx < settings.input_count) {
                active_input_idx = idx;
                stream.printf("Switched active input to %d\n", active_input_idx);
            }
            else
                stream.printf("Invalid input number. Up to %d are allowed.\n", settings.input_count-1);
        } else {
            switch (c) {
                case 'q': printCountDelta = !printCountDelta; break;
                case 'w': printCycles = !printCycles; prevCycleId = -1; break;
                case 'b': printFrames = !printFrames; break;
                case 'd': printDecoders = !printDecoders; break;
                case 'p': printPosition = !printPosition; break;
                case '+': changeCmpThreshold(active_input_idx, +1); stream.printf("Threshold level: %d\n", getCmpThreshold(active_input_idx)); break;
                case '-': changeCmpThreshold(active_input_idx, -1); stream.printf("Threshold level: %d\n", getCmpThreshold(active_input_idx)); break;
                case '!': settings.restart_in_configuration_mode(); break;
            }
        }
    }

    // Print different kinds of debug information.
    InputData &d = global_input_data[active_input_idx];
    if (printCountDelta) {
        stream.println();
        // TODO: Write pulse & cycle queue progress
        stream.printf("Threshold level: %d (%d)\n", getCmpThreshold(active_input_idx), getCmpLevel(active_input_idx));
    }

    if (printCycles) {
        Cycle c;
        while (d.cycles.dequeue(&c)) {
            if (c.phase_id == 0) {
                stream.println("\n==================================");
                prevCycleId = -1;
            }
            while (prevCycleId != -1 && prevCycleId+1 < (int)c.phase_id) {
                prevCycleId++;
                stream.printf("%d                  |", prevCycleId % 4);
                if (prevCycleId % 4 == 3)
                    stream.println();
            }
            prevCycleId = (int)c.phase_id;
            char ch = (d.fix_acquired && d.fix_cycle_offset == c.phase_id % 4) ? '*' : ' ';
            stream.printf("%d%c %2d %3d %3d %4d |", c.phase_id % 4, ch, c.cmp_threshold, c.first_pulse_len, c.second_pulse_len, c.laser_pulse_pos);
            if (prevCycleId % 4 == 3)
                stream.println();
        }
    }

    if (printDecoders) {
        for (int i = 0; i < num_big_pulses_in_cycle; i++) {
            Decoder &dec = d.decoders[i];
            stream.printf("DECODER %d: ", i);
            for (int j = 0; j < num_cycle_phases; j++) {
                BitDecoder &bit_dec = dec.bit_decoders[j];
                stream.printf("%3d, ", int(bit_dec.center_pulse_len));
            }
            stream.println();
        }
    }

    if (printFrames) {
        for (int i = 0; i < num_big_pulses_in_cycle; i++) {
            Decoder &dec = d.decoders[i];
            DataFrame frame;
            while (dec.data_frames.dequeue(&frame)) {
                stream.printf("FRAME %d (%d bytes):", i, frame.data_len);
                for (int j = 0; j < frame.data_len; j++)
                    stream.printf(" %02X", frame.data[j]);
                stream.println();
            }
        }
    }
}

// Update whether fix is acquired for given input.
void update_fix_acquired(InputData &d, uint32_t cur_millis) {
    // We say that we have a fix after 30 correct cycles and last correct cycle less than 100ms away.
    if (d.last_cycle_id > 30 && (cur_millis - d.last_cycle_time / 1000) < 100) {
        if (!d.fix_acquired) {
            bool invalid_pulse_lens = false;
            uint32_t min_idxes[2];
            for (int i = 0; i < num_big_pulses_in_cycle && !invalid_pulse_lens; i++) {
                Decoder &dec = d.decoders[i];
                
                // a. Find minimum len in bit_decoders
                uint32_t min_idx = 0;
                for (uint32_t j = 1; j < num_cycle_phases; j++)
                    if (dec.bit_decoders[j].center_pulse_len < dec.bit_decoders[min_idx].center_pulse_len)
                        min_idx = j;

                min_idxes[i] = min_idx;

                // b. Check that delta lens are as expected (10 - 30 - 10)
                float last_len = dec.bit_decoders[min_idx].center_pulse_len;
                static const int valid_deltas[num_cycle_phases - 1] = {10, 30, 10};
                for (uint32_t j = 0; j < num_cycle_phases - 1; j++) {
                    float cur_len = dec.bit_decoders[(min_idx+j+1) % num_cycle_phases].center_pulse_len;
                    if (abs(int(cur_len - last_len) - valid_deltas[j]) > 3) {
                        invalid_pulse_lens = true;
                        break;
                    }
                    last_len = cur_len;
                }
            }

            if (!invalid_pulse_lens && abs((int)min_idxes[0] - (int)min_idxes[1]) == 2) {
                // We've got a valid fix for both sources
                d.fix_acquired = true;
                d.fix_cycle_offset = min_idxes[0];
            }
        }
    } else {
        d.fix_acquired = false;
        for (int i = 0; i < num_cycle_phases; i++) {
            d.angle_timestamps[i] = 0;
            d.angle_lens[i] = 0;
        }
        d.angle_last_timestamp = 0;
        d.angle_last_processed_timestamp = 0;
    }
}

void process_pulse(uint32_t input_idx, uint32_t start_time, uint32_t pulse_len) {
    InputData &d = global_input_data[input_idx];
    Cycle &cur_cycle = d.cur_cycle;

    if (pulse_len >= max_big_pulse_len) {
        // Ignore it.
    } else if (pulse_len >= min_big_pulse_len) { // Wide pulse - likely sync pulse
        d.big_pulses++;
        uint32_t time_from_cycle_start = start_time - cur_cycle.start_time;
        uint32_t delta_second_cycle = time_from_cycle_start - (second_big_pulse_delay - 20);
        if (delta_second_cycle < 40) {  // We use that uint is > 0
            // Found second pulse
            cur_cycle.second_pulse_len = pulse_len;
            return;
        }

        // If there was a complete cycle before, we should write it and clean up.
        if (cur_cycle.start_time && time_from_cycle_start > (cycle_period-100)) {
            d.cycles.enqueue(cur_cycle);

            d.last_cycle_time = cur_cycle.start_time;
            d.last_cycle_id = cur_cycle.phase_id;

            extract_data_from_cycle(d, cur_cycle.first_pulse_len, cur_cycle.second_pulse_len, cur_cycle.phase_id);
            if (d.fix_acquired && cur_cycle.laser_pulse_pos > 0) {
                uint32_t adjusted_id = (cur_cycle.phase_id + 4 - d.fix_cycle_offset) % 4;
                d.angle_lens[adjusted_id] = cur_cycle.laser_pulse_pos - (adjusted_id >= 2 ? second_big_pulse_delay : 0);
                d.angle_timestamps[adjusted_id] = cur_cycle.start_time;
                d.angle_last_timestamp = cur_cycle.start_time;
            }
        }
        cur_cycle = {};

        // Check if this can be initial pulse
        int delay_from_last_cycle = int(start_time - d.last_cycle_time);
        if (delay_from_last_cycle < 200000) {
            uint32_t id = d.last_cycle_id;
            while (delay_from_last_cycle > cycle_period/2) {
                delay_from_last_cycle -= cycle_period;
                id++;
            }

            if (abs(delay_from_last_cycle) < 40) { // Good delay from last cycle, with period of cycle_period
                cur_cycle.start_time = start_time;
                cur_cycle.first_pulse_len = pulse_len;
                cur_cycle.phase_id = id;
                cur_cycle.cmp_threshold = getCmpThreshold(input_idx);
            } else {
                // Ignore current pulse, wait for other ones.
                d.fake_big_pulses++;
            }
        } else {
            // if delay from last successful cycle is more than 200ms then we lost tracking and we should try everything
            d.last_cycle_time = 0;
            d.last_cycle_id = 0;
            cur_cycle.start_time = start_time;
            cur_cycle.first_pulse_len = pulse_len;
            cur_cycle.phase_id = 0;
            cur_cycle.cmp_threshold = getCmpThreshold(input_idx);
        }

    } else if (pulse_len >= min_pulse_len) { // Short pulse - likely laser sweep
        d.small_pulses++;
        if (cur_cycle.start_time &&
                (start_time - cur_cycle.start_time) < cycle_period &&
                pulse_len > cur_cycle.laser_pulse_len) {
            cur_cycle.laser_pulse_len = pulse_len;
            cur_cycle.laser_pulse_pos = start_time + pulse_len/2 - cur_cycle.start_time;
        }
    } else { // Very short pulse - ignore.
        // TODO: Track it.        
    }
}

// This function is called by input methods when a new pulse is registered.
void add_pulse(uint32_t input_idx, uint32_t start_time, uint32_t end_time) {
    InputData &d = global_input_data[input_idx];
    d.pulses.enqueue({start_time, end_time-start_time});
}

bool have_valid_input_point(InputData &d, uint32_t cur_millis) {
    for (int i = 0; i < num_cycle_phases; i++)
        if (d.angle_timestamps[i]/1000 < cur_millis - 100)  // All angles were updated in the last 100ms.
            return false;
    return true;
}

void output_position(uint32_t input_idx, InputData &d, const float pos[3], float dist) {
    if (d.angle_last_timestamp == d.angle_last_processed_timestamp)
        return;

    if (printPosition && input_idx == active_input_idx)
        Serial.printf("POS: %.3f %.3f %.3f %.3f\n", pos[0], pos[1], pos[2], dist);
    
    // Covnert to NED coordinate system.
    float ned[3];
    convert_to_ned(pos, &ned);

    if (d.angle_last_timestamp - d.angle_last_processed_timestamp > 500000 ||
        (fabsf(ned[0] - d.last_ned[0]) < max_position_jump &&
            fabsf(ned[1] - d.last_ned[1]) < max_position_jump &&
            fabsf(ned[2] - d.last_ned[2]) < max_position_jump)) {

        if (input_idx == 0) {  // TODO: Support multiple inputs.
            // Point is valid; Send mavlink position.
            send_mavlink_position(ned);
            digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));
        }

        for (int i = 0; i < 3; i++)
            d.last_ned[i] = ned[i];
        d.angle_last_processed_timestamp = d.angle_last_timestamp;
    } else {
        Serial.printf("Invalid position: too far from previous one\n");
    }
}

// Main loop. All asynchronous calculations happen here.
void loop() {
    uint32_t cur_millis = millis();

    // Process debug I/O
    static uint32_t print_period = 0;
    if (throttle_ms(1000, cur_millis, &print_period))
        debug_io(Serial);

    // Blink once a second in normal mode without a fix.
    static uint32_t blink_period = 0;
    if (throttle_ms(1000, cur_millis, &blink_period)) {
        digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));
    }

    // Process pulses, cycles and output data.
    static uint32_t process_period = 0;
    if (throttle_ms(34, cur_millis, &process_period)) {  // 33.33ms => 4 cycles
        for (uint32_t input_idx = 0; input_idx < settings.input_count; input_idx++) {
            InputData &d = global_input_data[input_idx];

            // 1. Process pulses to generate cycles.
            Pulse p;
            while (d.pulses.dequeue(&p)) {
                process_pulse(input_idx, p.start_time, p.pulse_len);
            }

            // 2. Update fix state.
            update_fix_acquired(d, cur_millis);

            // 3. Calculate geometry & output it.
            if (d.fix_acquired && have_valid_input_point(d, cur_millis)) {
                if (settings.base_station_count >= 2) {
                    float pt[3], dist;
                    calculate_3d_point(d.angle_lens, &pt, &dist);
                    output_position(input_idx, d, pt, dist);
                } else if (printPosition && input_idx == active_input_idx) {
                    Serial.printf("Fix acquired, but cannot calculate position because base stations not configured.\n");
                }
            }
        }
    }
}
