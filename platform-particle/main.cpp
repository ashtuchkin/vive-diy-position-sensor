#include "vive_sensors_pipeline.h"
#include "settings.h"
#include "application.h"

SYSTEM_MODE(MANUAL);
//SerialLogHandler logHandler;  // Uncomment to pipe system logging to Serial.

// This will be either configuration pipeline, or production pipeline.
std::unique_ptr<Pipeline> pipeline;

void setup() {
    WiFi.connect();
    waitUntil(WiFi.ready);
}

void loop() {
    try {
        if (!pipeline || pipeline->is_stop_requested()) {
            if (settings.needs_configuration()) {
                // Initialize persistent settings interactively from user input, if needed.
                pipeline = settings.create_configuration_pipeline(0);
            } else {
                // Otherwise, create production pipeline.
                pipeline = create_vive_sensor_pipeline(settings);
            }
            pipeline->start();
        }
        
        pipeline->do_work(Timestamp::cur_time());
    }
    catch (const ValidationException &exc) {
        Serial.printf("Caught exception: %s", exc.what());
        PANIC(InvalidCase, "Caught exception: %s", exc.what());
    }
}
