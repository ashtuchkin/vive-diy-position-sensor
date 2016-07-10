#include "main.h"

// Comparator-specific data
input_data global_input_data[num_inputs] = {{
    // rise_start, dac_level, changes, noise_pulses, fake_big_pulses
    0, 10, 0,  0, 0,

    // last_cycle_time, id, cycle
    0, 0, {},

    // cycles.
    0, 0, {},

    // decoders
    {}
}};


// Print loop vars
bool printCountDelta = false, printCycles = false, printFrames = false, printDecoders = false;
unsigned int loopCount = 0;
unsigned int prevMillis = 0, curMillis;
int prevCycleId = -1;

void loop() {
    loopCount++;
    curMillis = millis();
    input_data &d = global_input_data[0];
    if (curMillis - prevMillis >= 1000) {
        prevMillis = curMillis;
        while (Serial.available()) {
            switch (Serial.read()) {
                case 'q': printCountDelta = !printCountDelta; break;
                case 'w': printCycles = !printCycles; prevCycleId = -1; break;
                case 'b': printFrames = !printFrames; break;
                case 'd': printDecoders = !printDecoders; break;
                case '+': setCmpDacLevel(d.dac_level++); Serial.printf("DAC level: %d\n", d.dac_level); break;
                case '-': setCmpDacLevel(d.dac_level--); Serial.printf("DAC level: %d\n", d.dac_level); break;
                default: break;
            }
        }

        if (printCycles) {
            for (; d.cycles_read_idx != d.cycles_write_idx; INC_CONSTRAINED(d.cycles_read_idx, cycles_buffer_len)) {
                cycle &c = d.cycles[d.cycles_read_idx];
                while (prevCycleId == -1 && prevCycleId+1 < (int)c.phase_id) {
                    prevCycleId++;
                    Serial.printf("        -      |");
                    if (prevCycleId % 4 == 3)
                        Serial.println();
                }
                prevCycleId = (int)c.phase_id;
                Serial.printf("%d %3d %3d %4d |", c.phase_id % 4, c.first_pulse_len, c.second_pulse_len, c.laser_pulse_pos);
                if (prevCycleId % 4 == 3)
                    Serial.println();
            }
        }

        if (printDecoders) {
            for (int i = 0; i < num_big_pulses_in_cycle; i++) {
                decoder &dec = d.decoders[i];
                Serial.printf("DECODER %d: ", i);
                for (int j = 0; j < num_cycle_phases; j++) {
                    bit_decoder &bit_dec = dec.bit_decoders[j];
                    Serial.printf("%3d, ", bit_dec.center_pulse_len >> 4);
                }
                Serial.println();
            }
        }

        if (printFrames) {
            for (int i = 0; i < num_big_pulses_in_cycle; i++) {
                decoder &dec = d.decoders[i];
                for (; dec.read_data_frames_idx != dec.write_data_frames_idx; INC_CONSTRAINED(dec.read_data_frames_idx, num_data_frames)) {
                    data_frame &frame = dec.data_frames[dec.read_data_frames_idx];
                    Serial.printf("FRAME %d (%d): ", i, frame.data_len);
                    for (int j = 0; j < frame.data_len; j++) {
                        Serial.print(frame.data[j], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
                }
            }
        }

        if (printCountDelta) {
            Serial.println();
            Serial.printf("Loops: %d\n", loopCount); loopCount = 0;

            Serial.printf("Changes: %d; %d; %d\n", d.changes, d.cycles_write_idx);
            Serial.printf("Cur level: %d\n", getCmpLevel());
        }

        digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));
    }
}

inline void process_pulse(input_data &d, uint32_t start_time, uint32_t pulse_len) {
    cycle &cur_cycle = d.cur_cycle;

    if (pulse_len > 50) { // Large pulse
        uint32_t time_from_cycle_start = start_time - cur_cycle.start_time;
        uint32_t delta_second_cycle = time_from_cycle_start - (second_big_pulse_delay - 20);
        if (delta_second_cycle < 40) {  // We use that uint is > 0
            // Found second pulse
            cur_cycle.second_pulse_len = pulse_len;
            return;
        }

        // If there was a complete cycle before, we should write it and clean up.
        if (cur_cycle.start_time && time_from_cycle_start > (cycle_period-100)) {
            d.cycles[d.cycles_write_idx] = cur_cycle;
            INC_CONSTRAINED(d.cycles_write_idx, cycles_buffer_len);

            d.last_cycle_time = cur_cycle.start_time;
            d.last_cycle_id = cur_cycle.phase_id;

            extract_data_from_cycle(d, cur_cycle.first_pulse_len, cur_cycle.second_pulse_len, cur_cycle.phase_id);
        }
        cur_cycle = {};

        // Check if this can be initial pulse
        int delay_from_last_cycle = int(start_time - d.last_cycle_time);
        if (delay_from_last_cycle < 100000) {
            uint32_t id = d.last_cycle_id;
            while (delay_from_last_cycle > cycle_period/2) {
                delay_from_last_cycle -= cycle_period;
                id++;
            }

            if (abs(delay_from_last_cycle) < 40) { // Good delay from last cycle, with period of cycle_period
                cur_cycle.start_time = start_time;
                cur_cycle.first_pulse_len = pulse_len;
                cur_cycle.phase_id = id;
            } else {
                // Ignore current pulse, wait for other ones.
                d.fake_big_pulses++;
            }
        } else {
            // if delay from last successful cycle is more than 100ms then we lost tracking and we should try everything
            cur_cycle.start_time = start_time;
            cur_cycle.first_pulse_len = pulse_len;
            cur_cycle.phase_id = 0;
        }

    } else if (pulse_len > min_pulse_len) { // Small pulse
        if (cur_cycle.start_time &&
                (start_time - cur_cycle.start_time) < cycle_period &&
                pulse_len > cur_cycle.laser_pulse_len) {
            cur_cycle.laser_pulse_len = pulse_len;
            cur_cycle.laser_pulse_pos = start_time + pulse_len/2 - cur_cycle.start_time;
        }
    } else {
        d.noise_pulses++;
    }
}


void cmp0_isr() {
    const uint32_t timestamp = micros();
    const uint32_t cmpState = CMP0_SCR;

    input_data &d = global_input_data[0];
    d.changes++;

    if (d.rise_time && (cmpState & CMP_SCR_CFF)) { // Fallen edge registered
        const uint32_t pulse_len = timestamp - d.rise_time;
        process_pulse(d, d.rise_time, pulse_len);
        d.rise_time = 0;
    }

    if (cmpState & (CMP_SCR_CFR | CMP_SCR_COUT)) { // Rising edge registered and state is now high
        d.rise_time = timestamp;
    }

    // Clear flags, re-enable interrupts.
    CMP0_SCR = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}


