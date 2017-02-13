cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE "teensy-arm.toolchain.cmake")
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
#set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(vive-diy-position-sensor)

set(SOURCE_FILES
        src/main.cpp
        src/settings.cpp
        
        src/cycle_phase_classifier.cpp
        src/data_frame_decoder.cpp
        src/debug_node.cpp
        src/geometry.cpp
        src/input.cpp
        src/input_cmp.cpp
        src/led_state.cpp
        src/mavlink.cpp
        src/outputs.cpp
        src/platform.cpp
        src/pulse_processor.cpp
        src/vive_sensors_pipeline.cpp
        
        src/primitives/timestamp.cpp
        src/primitives/string_utils.cpp
)

add_executable(vive-diy-position-sensor "${SOURCE_FILES}")
add_firmware_targets(vive-diy-position-sensor)

import_cmsis_dsp_library(vive-diy-position-sensor)
import_arduino_library(vive-diy-position-sensor mavlink_v2)
