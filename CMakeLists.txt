cmake_minimum_required(VERSION 3.2)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
#set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        src/cycle_phase_classifier.cpp
        src/data_frame_decoder.cpp
        src/formatters.cpp
        src/geometry.cpp
        src/input.cpp
        src/mavlink.cpp
        src/pulse_processor.cpp
        src/primitives/string_utils.cpp
)

set(PLATFORM_SPECIFIC_SOURCE_FILES
        src/main.cpp
        src/settings.cpp
        src/debug_node.cpp
        src/input_cmp.cpp
        src/led_state.cpp
        src/outputs.cpp
        src/platform.cpp
        src/vive_sensors_pipeline.cpp
        src/primitives/timestamp.cpp
)

set(TEST_SOURCE_FILES
        test/main_test.cpp
        test/platform_mocks.cpp
        test/test_pulse_processor.cpp
)

if (NOT TEST_MODE)
    set(CMAKE_TOOLCHAIN_FILE "teensy-arm.toolchain.cmake")

    project(vive-diy-position-sensor)

    add_executable(vive-diy-position-sensor "${SOURCE_FILES}" "${PLATFORM_SPECIFIC_SOURCE_FILES}")
    add_firmware_targets(vive-diy-position-sensor)

    import_cmsis_dsp_library(vive-diy-position-sensor)
    import_arduino_library(vive-diy-position-sensor mavlink_v2)

else()
    project(vive-diy-position-sensor)
    include(CTest)

    # Compile 32 bit code
    add_compile_options("-m32")
    link_libraries("-m32")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options("-Wno-deprecated-register" "-Wno-unknown-attributes")
    endif()

    # Compile CMSIS as a library.
    set(CMSIS_ROOT "${CMAKE_SOURCE_DIR}/libs/CMSIS/CMSIS" CACHE PATH "Path to the CMSIS root directory")
    file(GLOB_RECURSE CMSIS_CORE_FILES "${CMSIS_ROOT}/DSP_Lib/Source/*_f32.c")
    add_library(CMSIS "${CMSIS_CORE_FILES}" "${CMSIS_ROOT}/DSP_Lib/Source/CommonTables/arm_common_tables.c")
    target_compile_definitions(CMSIS PUBLIC "-DARM_MATH_CM4")
    link_libraries(CMSIS)

    # We have only one test executable
    add_definitions("-DNEW_H")  # Don't include new.h header as it defines non-standard operator new().
    include_directories("libs/Catch" "libs/CMSIS/CMSIS/Include" "libs/cores/teensy3" "libs/mavlink_v2" "src")
    add_executable(main-test "${TEST_SOURCE_FILES}" "${SOURCE_FILES}")
    add_test(main-test main-test)
endif()
