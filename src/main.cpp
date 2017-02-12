#include "vive_sensors_pipeline.h"
#include "settings.h"

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
