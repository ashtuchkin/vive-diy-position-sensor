#include "main.h"
#include "ring_buffer.h"

// Comparator-specific data
input_data global_input_data[num_inputs] = {{
    // rise_start, dac_level, fix_acquired, fix_cycle_offset
    // dac_level = 1V = (1.0/3.3*64) = 20
    0, 20, false, 0,

    // crossings, small_pulses, big_pulses, fake_big_pulses
    0, 0, 0, 0,

    // last_cycle_time, id, cycle
    0, 0, {},

    // cycles.
    0, 0, 0, {},

    // decoders
    {}
}};


// Print loop vars
bool printCountDelta = false, printCycles = false, printFrames = false, printDecoders = false;
bool printFTM1 = false, printPulses = false, printMicroseconds = false, useHardwareTimer = true;
unsigned int loopCount = 0, isrCount = 0;
unsigned int prevMillis = 0, prevMillis2 = 0, curMillis;
int32_t timebase_delta_ms = 0;

int prevCycleId = -1;

// Ring buffers for Lighthouse pulse processing
RingBuffer<uint32_t, 64> sensor0_start_tk;
RingBuffer<uint32_t, 64> sensor0_width_tk;

void loop() {
    loopCount++;
    curMillis = millis();
    input_data &d = global_input_data[0];

    process_incoming_mavlink_messages();

    if (curMillis - prevMillis >= 1000) {
        prevMillis = curMillis;
        while (Serial.available()) {
            switch (Serial.read()) {
                case 'q': printCountDelta = !printCountDelta; break;
                case 'w': printCycles = !printCycles; prevCycleId = -1; break;
                case 'b': printFrames = !printFrames; break;
                case 'd': printDecoders = !printDecoders; break;
                case '+': changeCmdDacLevel(d, +1); Serial.printf("DAC level: %d\n", d.dac_level); break;
                case '-': changeCmdDacLevel(d, -1); Serial.printf("DAC level: %d\n", d.dac_level); break;

                case 'a': printFTM1 = !printFTM1; break;
                case 's': printPulses = !printPulses; break;
                case 'u': printMicroseconds = !printMicroseconds; break;
                case 'h':
                    useHardwareTimer = !useHardwareTimer;

                    if (useHardwareTimer) {
                        NVIC_DISABLE_IRQ(IRQ_CMP0);
                    } else
                    {
                        NVIC_ENABLE_IRQ(IRQ_CMP0);
                    }
                    break;

                default: break;
            }
        }

        if (printFTM1) {
            Serial.printf("\nFTM1 ISR triggers: %d", isrCount);
            if (useHardwareTimer) Serial.printf("*");
        }

        // DEBUG: Print out the first 16 pulses for inspection
        // This will dequeue all pulses in the buffer from processing.
        if (printPulses) {
            int pulseIdx = 0;
            int value;
            while ((sensor0_width_tk.IsEmpty() != 1) && (pulseIdx < 16)) {
                if (pulseIdx % 4 == 0) {
                    Serial.print("\nPulse widths ");
                    printMicroseconds ? Serial.print("(us): "):
                                        Serial.print("(tk): ");
                }

                value = sensor0_width_tk.PopBack();

                // TODO: Correct for specific master clock speed, assumes 48 MHz
                if (printMicroseconds) value = value / 48;
                Serial.printf("%4d ", value);
                pulseIdx++;
            }

            // Count up the remaining pulses
            while((sensor0_width_tk.IsEmpty() != 1))
            {
                sensor0_width_tk.PopBack();
                pulseIdx++;
            }

            Serial.printf("\nPulses in buffer: %d", pulseIdx);
            Serial.printf("\nLast pulse timestamp (tk): %u", sensor0_start_tk.PopBack());
        }

        if (printCycles) {
            for (; d.cycles_read_idx != d.cycles_write_idx; INC_CONSTRAINED(d.cycles_read_idx, cycles_buffer_len)) {
                cycle &c = d.cycles[d.cycles_read_idx];
                if (c.phase_id == 0) {
                    Serial.println("\n==================================");
                    prevCycleId = -1;
                }
                while (prevCycleId != -1 && prevCycleId+1 < (int)c.phase_id) {
                    prevCycleId++;
                    Serial.printf("%d                  |", prevCycleId % 4);
                    if (prevCycleId % 4 == 3)
                        Serial.println();
                }
                prevCycleId = (int)c.phase_id;
                char ch = (d.fix_acquired && d.fix_cycle_offset == c.phase_id % 4) ? '*' : ' ';
                Serial.printf("%d%c %2d %3d %3d %4d |", c.phase_id % 4, ch, c.dac_level, c.first_pulse_len, c.second_pulse_len, c.laser_pulse_pos);
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
            Serial.printf("Cycles write idx: %d\n", d.cycles_write_idx);
            Serial.printf("Cur Dac level: %d (%d)\n", d.dac_level, getCmpLevel());
            Serial.printf("Crossings: %d", d.crossings);
        }

        digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));
    }

    if (curMillis - prevMillis2 >= 34) {  // 33.33ms => 4 cycles
        prevMillis2 = curMillis;

        // Process pulses outside of the ISR
        if (useHardwareTimer)
        {
            // Need to rebase timebase used by the remainder of pulse timing code
            // Difference between the two timebases in milliseconds
            timebase_delta_ms = curMillis - ((FTM1_CNT | (ftm1_overflow << 16)) / 48000);

            // Dequeue pending pulses
            while (sensor0_width_tk.IsEmpty() != 1){
                d.crossings++;
                d.rise_time = sensor0_start_tk.PopBack();

                // WARNING: process_pulse() currently uses microseconds as the
                // unit for pulse start and pulse width. This is a 48x loss of
                // precision than could be obtained using HW timer with ticks
                // of 48 MHz clock (20.83 ns per tick).
                process_pulse(d, (d.rise_time / 48) + timebase_delta_ms, (sensor0_width_tk.PopBack() / 48) + timebase_delta_ms);
            }
        }

        /*
        // 1. Comparator level dynamic adjustment.
        __disable_irq();
        uint32_t crossings = d.crossings; d.crossings = 0;
        uint32_t big_pulses = d.big_pulses; d.big_pulses = 0;
        uint32_t small_pulses = d.small_pulses; d.small_pulses = 0;
        //uint32_t fake_big_pulses = d.fake_big_pulses; d.fake_big_pulses = 0;
        __enable_irq();

        if (crossings > 100) { // Comparator level is at the background noise level. Try to increase it.
            changeCmdDacLevel(d, +3);

        } else if (crossings == 0) { // No crossings of comparator level => too high or too low.
            if (getCmpLevel() == 1) { // Level is too low
                changeCmdDacLevel(d, +16);
            } else { // Level is too high
                changeCmdDacLevel(d, -9);
            }
        } else {

            if (big_pulses <= 6) {
                changeCmdDacLevel(d, -4);
            } else if (big_pulses > 10) {
                changeCmdDacLevel(d, +4);
            } else if (small_pulses < 4) {

                // Fine tune - we need 4 small pulses.
                changeCmdDacLevel(d, -1);
            }
        }
        */

        // 2. Fix flag & fix_cycle_offset
        if (d.last_cycle_id > 30 && (curMillis - d.last_cycle_time / 1000) < 100) {
            if (!d.fix_acquired) {
                bool invalid_pulse_lens = false;
                uint32_t min_idxes[2];
                for (int i = 0; i < num_big_pulses_in_cycle && !invalid_pulse_lens; i++) {
                    decoder &dec = d.decoders[i];

                    // a. Find minimum len in bit_decoders
                    uint32_t min_idx = 0;
                    for (uint32_t j = 1; j < num_cycle_phases; j++)
                        if (dec.bit_decoders[j].center_pulse_len < dec.bit_decoders[min_idx].center_pulse_len)
                            min_idx = j;

                    min_idxes[i] = min_idx;

                    // b. Check that delta lens are as expected (10 - 30 - 10)
                    uint32_t cur_idx = min_idx;
                    int last_len = dec.bit_decoders[cur_idx].center_pulse_len;
                    static const int valid_deltas[num_cycle_phases - 1] = {10, 30, 10};
                    for (uint32_t j = 0; j < num_cycle_phases - 1; j++) {
                        INC_CONSTRAINED(cur_idx, num_cycle_phases);
                        int cur_len = dec.bit_decoders[cur_idx].center_pulse_len;
                        if (abs(((cur_len - last_len) >> 4) - valid_deltas[j]) > 3) {
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
                    d.cycles_read_geom_idx = d.cycles_write_idx;
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

        // 3. Read current angles and calculate geometry
        if (d.fix_acquired && d.angle_last_timestamp > d.angle_last_processed_timestamp) {
            bool have_valid_input_point = true;
            for (int i = 0; i < num_cycle_phases && have_valid_input_point; i++)
                if (d.angle_timestamps[i]/1000 < curMillis - 100)
                    have_valid_input_point = false;

            if (have_valid_input_point) {

                // Convert timing to 3d coordinates in NED frame.
                float ned[3], dist;
                calculate_3d_point(d, &ned, &dist);

                digitalWriteFast(LED_BUILTIN, (uint8_t)(!digitalReadFast(LED_BUILTIN)));
                Serial.printf("Position: %.3f %.3f %.3f ; dist= %.3f\n", ned[0], ned[1], ned[2], dist);
                
                if (d.angle_last_timestamp - d.angle_last_processed_timestamp > 500000 ||
                       (fabsf(ned[0] - d.last_ned[0]) < max_position_jump &&
                        fabsf(ned[1] - d.last_ned[1]) < max_position_jump &&
                        fabsf(ned[2] - d.last_ned[2]) < max_position_jump)) {

                    // Point is valid; Send position.
                    send_mavlink_position(ned);
                    for (int i = 0; i < 3; i++)
                        d.last_ned[i] = ned[i];
                    d.angle_last_processed_timestamp = d.angle_last_timestamp;
                } else {
                    Serial.printf("Invalid position: too far from previous one");
                }
            }
        }
    }
}

inline void process_pulse(input_data &d, uint32_t start_time, uint32_t pulse_len) {
    cycle &cur_cycle = d.cur_cycle;

    if (pulse_len >= max_big_pulse_len) {
        // Ignore it.
    } else if (pulse_len >= min_big_pulse_len) { // Large pulse
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
            d.cycles[d.cycles_write_idx] = cur_cycle;
            INC_CONSTRAINED(d.cycles_write_idx, cycles_buffer_len);

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
                cur_cycle.dac_level = d.dac_level;
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
            cur_cycle.dac_level = d.dac_level;
        }

    } else if (pulse_len >= min_pulse_len) { // Small pulse - probably laser sweep
        d.small_pulses++;
        if (cur_cycle.start_time &&
                (start_time - cur_cycle.start_time) < cycle_period &&
                pulse_len > cur_cycle.laser_pulse_len) {
            cur_cycle.laser_pulse_len = pulse_len;
            cur_cycle.laser_pulse_pos = start_time + pulse_len/2 - cur_cycle.start_time;
        }
    }
}

void cmp0_isr() {
    const uint32_t timestamp = micros();
    const uint32_t cmpState = CMP0_SCR;

    input_data &d = global_input_data[0];
    d.crossings++;

    if (d.rise_time && (cmpState & (sensorActiveHigh ? CMP_SCR_CFF : CMP_SCR_CFR))) { // Fallen edge registered
        const uint32_t pulse_len = timestamp - d.rise_time;
        process_pulse(d, d.rise_time, pulse_len);
        d.rise_time = 0;
    }

    if (cmpState & ((sensorActiveHigh ? CMP_SCR_CFR : CMP_SCR_CFF) | CMP_SCR_COUT)) { // Rising edge registered and state is now high
        d.rise_time = timestamp;
    }

    // Clear flags, re-enable interrupts.
    CMP0_SCR = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}


extern "C" void FASTRUN ftm1_isr(void) {
    uint32_t first_edge_tk, second_edge_tk, pulse_width_tk;

    // Handle timer overflow in timestamps
    if (FTM1_SC & FTM_SC_TOF) {
        ++ftm1_overflow;
        if (ftm1_overflow == 0) ++ftm1_overflow_89s;
        FTM1_SC &= ~FTM_SC_TOF;
    }

    if (FTM1_C0SC & FTM_CSC_CHF) {
        // Technically do nothing
        // FTM1_C0SC &= ~FTM_CSC_CHF;
    }

    if (FTM1_C1SC & FTM_CSC_CHF) {
        uint16_t c0v = FTM1_C0V;
        uint16_t c1v = FTM1_C1V;

        first_edge_tk = c0v | (ftm1_overflow << 16);
        second_edge_tk = c1v | (ftm1_overflow << 16);
        FTM1_C1SC &= ~FTM_CSC_CHF;

        // Need to handle tricky overflow situation
        // Not properly capturing the CH0-TOF-CH1 event sequence
        if (second_edge_tk < first_edge_tk) {
            first_edge_tk = (c0v | ftm1_overflow << 16) - (1 << 16);
        }

        pulse_width_tk = second_edge_tk - first_edge_tk;
        sensor0_start_tk.PushFront(first_edge_tk);
        sensor0_width_tk.PushFront(pulse_width_tk);

        ++isrCount;
    }
    return;
}