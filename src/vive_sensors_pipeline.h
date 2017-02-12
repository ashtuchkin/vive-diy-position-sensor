#pragma once
#include <memory>

class PersistentSettings;
class Pipeline;

// Create Pipeline specialized for Vive Sensor, using provided configuration settings.
std::unique_ptr<Pipeline> create_vive_sensor_pipeline(const PersistentSettings &settings);
