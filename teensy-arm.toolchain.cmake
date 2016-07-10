# Teensy 3.1 toolchain, inspired by:
#  * https://github.com/xya/teensy-cmake
#  * http://playground.arduino.cc/Code/CmakeBuild

# Toolchain definition
set(TOOLCHAIN_PREFIX "/usr/local/bin/arm-none-eabi-")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING 1)

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc" CACHE PATH "gcc" FORCE)
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++" CACHE PATH "g++" FORCE)
set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar" CACHE PATH "ar" FORCE)
set(CMAKE_LINKER       "${TOOLCHAIN_PREFIX}ld" CACHE PATH "ld" FORCE)
set(CMAKE_NM           "${TOOLCHAIN_PREFIX}nm" CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}objcopy" CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PREFIX}objdump" CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP        "${TOOLCHAIN_PREFIX}strip" CACHE PATH "strip" FORCE)
set(CMAKE_PRINT_SIZE   "${TOOLCHAIN_PREFIX}size" CACHE PATH "size" FORCE)
set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib" CACHE PATH "ranlib" FORCE)


# Teensy 3.1 settings

set(TEENSY_ROOT "${CMAKE_SOURCE_DIR}/libs/teensy3" CACHE PATH "Path to the Teensy 'cores/teensy3' repository")

# /Users/<username>/Documents/Arduino/libraries
# /Applications/Arduino.app/Contents/Java/libraries
set(ARDUINO_LIB_ROOT "${CMAKE_SOURCE_DIR}/libs" CACHE PATH "Path to the Arduino library directory")
set(ARDUINO_VERSION "10607" CACHE STRING "Version of the Arduino SDK")

set(TEENSYDUINO_VERSION "127" CACHE STRING "Version of the Teensyduino SDK")
set(TEENSY_MODEL "MK20DX256" CACHE STRING "Model of the Teensy MCU")

set(TEENSY_FREQUENCY "96" CACHE STRING "Frequency of the Teensy MCU (Mhz)")
set_property(CACHE TEENSY_FREQUENCY PROPERTY STRINGS 96 72 48 24 16 8 4 2)

set(TEENSY_USB_MODE "SERIAL" CACHE STRING "What kind of USB device the Teensy should emulate")
set_property(CACHE TEENSY_USB_MODE PROPERTY STRINGS SERIAL HID SERIAL_HID MIDI RAWHID FLIGHTSIM)


set(TARGET_FLAGS "-mcpu=cortex-m4 -mthumb -mfp16-format=ieee")
set(OPTIMIZE_FLAGS "-O2" CACHE STRING "Optimization flags")  # Remember to reset cache and rebuild cmake when changing this.
set(CMAKE_C_FLAGS "${OPTIMIZE_FLAGS} -Wall -nostdlib -ffunction-sections -fdata-sections ${TARGET_FLAGS}" CACHE STRING "C/C++ flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti -felide-constructors -std=gnu++0x -fsingle-precision-constant" CACHE STRING "c++ flags")

# TODO: https://github.com/ARM-software/CMSIS
# link_libraries(arm_cortexM4l_math)

link_libraries(m)
set(LINKER_FLAGS "--gc-sections,--relax,--defsym=__rtc_localtime=0" CACHE STRING "Ld flags")
set(CXX_LINKER_FLAGS "${OPTIMIZE_FLAGS} -Wl,${LINKER_FLAGS} ${TARGET_FLAGS} -T${TEENSY_ROOT}/mk20dx256.ld")

set(CMAKE_SHARED_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Linker flags" FORCE)

# Don't pass regular CMAKE_CXX_FLAGS because that causes undefined symbols
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")


add_definitions("-DARDUINO=${ARDUINO_VERSION}")
add_definitions("-DTEENSYDUINO=${TEENSYDUINO_VERSION}")
add_definitions("-D__${TEENSY_MODEL}__")
add_definitions("-DUSB_${TEENSY_USB_MODE}")
add_definitions("-DF_CPU=${TEENSY_FREQUENCY}000000")
add_definitions("-DLAYOUT_US_ENGLISH")
add_definitions("-MMD")


# See https://github.com/Koromix/ty
set(TY_EXECUTABLE "/usr/local/bin/tyc" CACHE FILEPATH "Path to the 'tyc' executable that can upload programs to the Teensy")

# Define target for the Teensy 'core' library.
# CMake does a lot of compiler checks which include toolchain file, so be careful to define it only once and only
# for the actual source (not sample programs).
if (NOT TARGET TeensyCore AND NOT ${CMAKE_SOURCE_DIR} MATCHES "CMakeTmp")
    file(GLOB TEENSY_C_CORE_FILES "${TEENSY_ROOT}/*.c")
    file(GLOB TEENSY_CXX_CORE_FILES "${TEENSY_ROOT}/*.cpp")
    add_library(TeensyCore ${TEENSY_C_CORE_FILES} ${TEENSY_CXX_CORE_FILES})
    link_libraries(TeensyCore)
    include_directories(${TEENSY_ROOT})

    # Add CMSIS DSP library
    link_directories(${TEENSY_ROOT})
    link_libraries(arm_cortexM4l_math)
endif (NOT TARGET TeensyCore AND NOT ${CMAKE_SOURCE_DIR} MATCHES "CMakeTmp")


function(add_firmware_targets TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE "${TEENSY_ROOT}")

    # Generate the hex firmware files that can be flashed to the MCU.
    set(EEPROM_OPTS -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0)
    set(HEX_OPTS -O ihex -R .eeprom)
    add_custom_target(${TARGET_NAME}_Firmware ALL
            COMMENT "Firmware size:"
            COMMAND ${CMAKE_PRINT_SIZE} $<TARGET_FILE_NAME:${TARGET_NAME}> | tail -1 | xargs bash -c "TEXT=$0; DATA=$1; BSS=$2; TOTAL_FLASH=262144; TOTAL_RAM=65536; FLASH=$((TEXT+DATA)); RAM=$((DATA+BSS)); echo \"FLASH: $FLASH ($((FLASH*100/TOTAL_FLASH))%); RAM: $RAM ($((RAM*100/TOTAL_RAM))%); Free RAM: $((TOTAL_RAM-RAM))\";"
            COMMAND ${CMAKE_OBJCOPY} ${EEPROM_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.eep
            COMMAND ${CMAKE_OBJCOPY} ${HEX_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.hex
            VERBATIM)

    add_custom_target(${TARGET_NAME}_Assembler
            COMMAND ${CMAKE_OBJDUMP} -d $<TARGET_FILE_NAME:${TARGET_NAME}> > ${CMAKE_SOURCE_DIR}/build/source.S )

    if(EXISTS "${TY_EXECUTABLE}")
        add_custom_target(${TARGET_NAME}_Upload
                COMMAND "${TY_EXECUTABLE}" upload ${TARGET_NAME}.hex)
        add_dependencies(${TARGET_NAME}_Upload ${TARGET_NAME}_Firmware)
    endif(EXISTS "${TY_EXECUTABLE}")
endfunction(add_firmware_targets)


function(import_arduino_library TARGET_NAME LIB_NAME)
    # Check if we can find the library.
    set(LIB_DIR "${ARDUINO_LIB_ROOT}/${LIB_NAME}")
    if(NOT EXISTS "${LIB_DIR}")
        message(FATAL_ERROR "Could not find the directory for library '${LIB_NAME}'")
    endif(NOT EXISTS "${LIB_DIR}")

    # Add it to the include path.
    target_include_directories(${TARGET_NAME} PRIVATE "${LIB_DIR}")

    # Mark source files to be built along with the sketch code.
    file(GLOB SOURCES_CPP ABSOLUTE "${LIB_DIR}" "${LIB_DIR}/*.cpp")
    file(GLOB SOURCES_C ABSOLUTE "${LIB_DIR}" "${LIB_DIR}/*.c")
    target_sources(${TARGET_NAME} PRIVATE "${SOURCES_CPP}")
    target_sources(${TARGET_NAME} PRIVATE "${SOURCES_C}")
endfunction(import_arduino_library)
