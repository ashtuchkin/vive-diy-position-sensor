cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE "teensy-arm.toolchain.cmake")
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
#set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(vive_sensor_teensy)

set(SOURCE_FILES
        main.cpp
        setup.cpp
        data_decoder.cpp
        geometry.cpp
        mavlink.cpp
)

add_executable(vive-sensor-teensy "${SOURCE_FILES}")
add_firmware_targets(vive-sensor-teensy)

import_cmsis_dsp_library(vive-sensor-teensy)
import_arduino_library(vive-sensor-teensy mavlink_v2)
