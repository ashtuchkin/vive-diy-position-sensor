cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE "teensy-arm.toolchain.cmake")
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
#set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(vive-diy-position-sensor)

set(SOURCE_FILES
        main.cpp
        setup.cpp
        input_cmp.cpp
        settings.cpp
        data_decoder.cpp
        geometry.cpp
        mavlink.cpp
        util.cpp
)

add_executable(vive-diy-position-sensor "${SOURCE_FILES}")
add_firmware_targets(vive-diy-position-sensor)

import_cmsis_dsp_library(vive-diy-position-sensor)
import_arduino_library(vive-diy-position-sensor mavlink_v2)
