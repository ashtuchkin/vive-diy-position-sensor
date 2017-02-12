#include "vive_sensors_pipeline.h"
#include "settings.h"

/*
            // 3. Calculate geometry & output it.
            if (d.fix_acquired && have_valid_input_point(d, cur_time)) {
                if (settings.base_station_count >= 2) {
                    float pt[3], dist;
                    calculate_3d_point(d.angle_lens, &pt, &dist);
                    output_position(input_idx, d, pt, dist);
                } else if (printPosition && input_idx == active_input_idx) {
                    Serial.printf("Fix acquired, but cannot calculate position because base stations not configured.\n");
                }
            }

const static float max_position_jump = 0.05; // meters

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
*/

#include <avr_emulation.h>
#include <usb_serial.h>


extern "C" int main() {
    // Initialize core devices.
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize persistent settings interactively from user input, if needed.
    if (settings.needs_configuration()) {
        digitalWrite(LED_BUILTIN, HIGH);
        settings.initialize_from_user_input(Serial);
    }

    // Create worker node pipeline from settings.
    auto pipeline = create_vive_sensor_pipeline(settings);

    // Register & start input nodes' interrupts.
    pipeline->start();

    // Main loop.
    while (true) {
        // Process pulses, cycles and output data.
        pipeline->do_work(Timestamp::cur_time());
    }
}
