set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        cycle_phase_classifier.cpp
        data_frame_decoder.cpp
        debug_node.cpp
        formatters.cpp
        geometry.cpp
        input.cpp
        mavlink.cpp
        outputs.cpp
        pulse_processor.cpp
        settings.cpp
        vive_sensors_pipeline.cpp

        primitives/string_utils.cpp
        primitives/timestamp.cpp
)

add_library(sensor-core STATIC EXCLUDE_FROM_ALL ${SOURCE_FILES})
target_include_directories(sensor-core PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/libs/mavlink_v2)
target_link_libraries(sensor-core PRIVATE cmsis)

target_include_directories(sensor-core PUBLIC ${CMAKE_SOURCE_DIR}/include)
