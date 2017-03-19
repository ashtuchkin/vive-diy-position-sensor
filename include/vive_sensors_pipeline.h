#pragma once
#include "primitives/workers.h"
#include "settings.h"
#include <memory>

// Create Pipeline specialized for Vive Sensors, using provided configuration settings.
std::unique_ptr<Pipeline> create_vive_sensor_pipeline(const PersistentSettings &settings);
