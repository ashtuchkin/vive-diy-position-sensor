set(CMAKE_CXX_STANDARD 14)

set(TEENSY_SOURCE_FILES
        main.cpp
        platform.cpp
        timestamp.cpp
        led_state.cpp

        input_cmp.cpp
        input_ftm.cpp
        output_serial.cpp
)

include(teensy-targets.cmake)

add_executable(vive-diy-position-sensor "${TEENSY_SOURCE_FILES}")
target_link_libraries(vive-diy-position-sensor PRIVATE sensor-core teensy-core)
add_firmware_targets(vive-diy-position-sensor)

