
# Teensy/Arduino definitions
# /Users/<username>/Documents/Arduino/libraries
# /Applications/Arduino.app/Contents/Java/libraries
set(ARDUINO_LIB_ROOT "${CMAKE_SOURCE_DIR}/libs" CACHE PATH "Path to the Arduino library directory")
set(ARDUINO_VERSION "10607" CACHE STRING "Version of the Arduino SDK")

set(TEENSYDUINO_VERSION "127" CACHE STRING "Version of the Teensyduino SDK")
set(TEENSY_MODEL "MK20DX256" CACHE STRING "Model of the Teensy MCU")
set(TEENSY_FREQUENCY "96" CACHE STRING "Frequency of the Teensy MCU (Mhz)")
set(TEENSY_USB_MODE "SERIAL" CACHE STRING "What kind of USB device the Teensy should emulate")
set_property(CACHE TEENSY_USB_MODE PROPERTY STRINGS SERIAL HID SERIAL_HID MIDI RAWHID FLIGHTSIM)

add_definitions("-DARDUINO=${ARDUINO_VERSION}")
add_definitions("-DTEENSYDUINO=${TEENSYDUINO_VERSION}")
add_definitions("-D__${TEENSY_MODEL}__")
add_definitions("-DUSB_${TEENSY_USB_MODE}")
add_definitions("-DF_CPU=${TEENSY_FREQUENCY}000000")
add_definitions("-DLAYOUT_US_ENGLISH")


# Teensy core library
file(GLOB TEENSY_C_CORE_FILES "${TEENSY_ROOT}/*.c")
list(REMOVE_ITEM TEENSY_C_CORE_FILES "${TEENSY_ROOT}/math_helper.c")  # legacy cmsis file - not needed anyway.
file(GLOB TEENSY_CXX_CORE_FILES "${TEENSY_ROOT}/*.cpp")
list(REMOVE_ITEM TEENSY_CXX_CORE_FILES "${TEENSY_ROOT}/new.cpp")  # Don't use non-standard operator new.

add_library(teensy-core STATIC EXCLUDE_FROM_ALL ${TEENSY_C_CORE_FILES} ${TEENSY_CXX_CORE_FILES})
target_include_directories(teensy-core PUBLIC ${TEENSY_ROOT})
target_compile_definitions(teensy-core PUBLIC "NEW_H")  # Don't include new.h header as it defines non-standard operator new().

add_custom_command(TARGET teensy-core POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --weaken-symbol=_sbrk $<TARGET_FILE:teensy-core>
        COMMENT Allow replacement of _sbrk() to better control memory allocation
)


# CMSIS library
set(CMSIS_ROOT "${CMAKE_SOURCE_DIR}/libs/CMSIS/CMSIS" CACHE PATH "Path to the CMSIS root directory")
add_library(cmsis STATIC IMPORTED GLOBAL)
set_target_properties(
    cmsis PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMSIS_ROOT}/Include"
    INTERFACE_COMPILE_DEFINITIONS "ARM_MATH_CM4"
    IMPORTED_LOCATION "${CMSIS_ROOT}/Lib/GCC/libarm_cortexM4l_math.a"
)


# Firmare helper targets
function(add_firmware_targets TARGET_NAME)
    # Generate the hex firmware files that can be flashed to the MCU.
    set(EEPROM_OPTS -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0)
    set(HEX_OPTS -O ihex -R .eeprom)

    if(NOT (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows"))
        set(PROCESS_SIZE_CMD_OUTPUT | tail -1 | xargs bash -c [[ TEXT=$0\; DATA=$1\; BSS=$2\; TOTAL_FLASH=262144\; TOTAL_RAM=65536\; FLASH=$((TEXT+DATA))\; RAM=$((DATA+BSS))\; echo "FLASH: $FLASH ($((FLASH*100/TOTAL_FLASH))%), RAM: $RAM ($((RAM*100/TOTAL_RAM))%), Free RAM: $((TOTAL_RAM-RAM))" ]])
    endif()

    add_custom_target(${TARGET_NAME}-firmware ALL
            COMMENT "Firmware size:"
            COMMAND ${CMAKE_PRINT_SIZE} $<TARGET_FILE_NAME:${TARGET_NAME}> ${PROCESS_SIZE_CMD_OUTPUT}
            COMMAND ${CMAKE_OBJCOPY} ${EEPROM_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.eep
            COMMAND ${CMAKE_OBJCOPY} ${HEX_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.hex
            VERBATIM
    )

    add_custom_target(${TARGET_NAME}-source
            COMMAND ${CMAKE_OBJDUMP} --demangle --disassemble --headers --wide $<TARGET_FILE_NAME:${TARGET_NAME}> > ${CMAKE_SOURCE_DIR}/build/source.S
            COMMAND ${CMAKE_OBJDUMP} --demangle --disassemble --source --wide $<TARGET_FILE_NAME:${TARGET_NAME}> > ${CMAKE_SOURCE_DIR}/build/source-with-text.S
            COMMAND ${CMAKE_OBJDUMP} --demangle --syms $<TARGET_FILE_NAME:${TARGET_NAME}> | sort > ${CMAKE_SOURCE_DIR}/build/source-symbols.txt
    )

    # See https://github.com/Koromix/ty
    find_file(TY_EXECUTABLE tyc 
        PATHS "/usr/local/bin" 
        DOC "Path to the 'tyc' executable that can upload programs to the Teensy")

    if(TY_EXECUTABLE)
        add_custom_target(${TARGET_NAME}-upload
                COMMAND "${TY_EXECUTABLE}" upload ${TARGET_NAME}.hex)
        add_dependencies(${TARGET_NAME}-upload ${TARGET_NAME}-firmware)
    endif()
endfunction(add_firmware_targets)
