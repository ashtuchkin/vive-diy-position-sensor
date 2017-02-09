# Teensy 3.1 toolchain, inspired by:
#  * https://github.com/xya/teensy-cmake
#  * http://playground.arduino.cc/Code/CmakeBuild

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING 1)

set(TOOLCHAIN_TRIPLE "arm-none-eabi-" CACHE STRING "Triple prefix for arm crosscompiling tools")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(TOOLCHAIN_SUFFIX ".exe" CACHE STRING "Toolchain executable file extension")
else()
    set(TOOLCHAIN_SUFFIX "" CACHE STRING "Toolchain executable file extension")
endif()

# Search default paths for the GNU ARM Embedded Toolchain (https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)
# This will need to be changed if you have it in a different directory.
find_path(TOOLCHAIN_BIN_PATH "${TOOLCHAIN_TRIPLE}gcc${TOOLCHAIN_SUFFIX}"
    PATHS "/usr/local/bin"                                       # Linux, Mac
    PATHS "C:/Program Files (x86)/GNU Tools ARM Embedded/*/bin"  # Default installation directory on Windows
    DOC "Toolchain binaries directory")

if(NOT TOOLCHAIN_BIN_PATH)
    message(FATAL_ERROR "GNU ARM Embedded Toolchain not found. Please install it and provide its /bin directory as a TOOLCHAIN_BIN_PATH variable.")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}gcc${TOOLCHAIN_SUFFIX}" CACHE PATH "gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}g++${TOOLCHAIN_SUFFIX}" CACHE PATH "g++")
set(CMAKE_AR           "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ar${TOOLCHAIN_SUFFIX}" CACHE PATH "ar")
set(CMAKE_LINKER       "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ld${TOOLCHAIN_SUFFIX}" CACHE PATH "ld")
set(CMAKE_NM           "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}nm${TOOLCHAIN_SUFFIX}" CACHE PATH "nm")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}objcopy${TOOLCHAIN_SUFFIX}" CACHE PATH "objcopy")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}objdump${TOOLCHAIN_SUFFIX}" CACHE PATH "objdump")
set(CMAKE_STRIP        "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}strip${TOOLCHAIN_SUFFIX}" CACHE PATH "strip")
set(CMAKE_PRINT_SIZE   "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}size${TOOLCHAIN_SUFFIX}" CACHE PATH "size")
set(CMAKE_RANLIB       "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ranlib${TOOLCHAIN_SUFFIX}" CACHE PATH "ranlib")


# Teensy 3.1 settings

set(TEENSY_ROOT "${CMAKE_SOURCE_DIR}/libs/cores/teensy3" CACHE PATH "Path to the Teensy 'cores/teensy3' repository")
set(CMSIS_ROOT "${CMAKE_SOURCE_DIR}/libs/CMSIS/CMSIS" CACHE PATH "Path to the CMSIS root directory")

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
set(CMAKE_C_FLAGS "${OPTIMIZE_FLAGS} -Wall -ffunction-sections -fdata-sections ${TARGET_FLAGS}" CACHE STRING "C/C++ flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++14 -fno-rtti -fsingle-precision-constant -Woverloaded-virtual" CACHE STRING "C++ flags")

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "" FORCE)  # Don't do -O3 because it increases the size. Just remove asserts.
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "" FORCE)

link_libraries(m)
set(LINKER_FLAGS "--gc-sections,--relax,--defsym=__rtc_localtime=0" CACHE STRING "Ld flags")
set(CXX_LINKER_FLAGS "${OPTIMIZE_FLAGS} -Wl,${LINKER_FLAGS} ${TARGET_FLAGS} -T${TEENSY_ROOT}/mk20dx256.ld")

set(CMAKE_SHARED_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Shared Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Module Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Executable Linker flags" FORCE)


add_definitions("-DARDUINO=${ARDUINO_VERSION}")
add_definitions("-DTEENSYDUINO=${TEENSYDUINO_VERSION}")
add_definitions("-D__${TEENSY_MODEL}__")
add_definitions("-DUSB_${TEENSY_USB_MODE}")
add_definitions("-DF_CPU=${TEENSY_FREQUENCY}000000")
add_definitions("-DLAYOUT_US_ENGLISH")
add_definitions("-DNEW_H")  # Don't include new.h header as it defines non-standard operator new().
add_definitions("-MMD")

# Define target for the Teensy 'core' library.
# CMake does a lot of compiler checks which include toolchain file, so be careful to define it only once and only
# for the actual source (not sample programs).
if (NOT TARGET TeensyCore AND NOT ${CMAKE_SOURCE_DIR} MATCHES "CMakeTmp")
    file(GLOB TEENSY_C_CORE_FILES "${TEENSY_ROOT}/*.c")
    list(REMOVE_ITEM TEENSY_C_CORE_FILES "${TEENSY_ROOT}/math_helper.c")  # legacy cmsis file - not needed anyway.
    file(GLOB TEENSY_CXX_CORE_FILES "${TEENSY_ROOT}/*.cpp")
    list(REMOVE_ITEM TEENSY_CXX_CORE_FILES "${TEENSY_ROOT}/new.cpp")  # Don't use non-standard operator new.
    add_library(TeensyCore ${TEENSY_C_CORE_FILES} ${TEENSY_CXX_CORE_FILES})
    link_libraries(TeensyCore)
    include_directories(${TEENSY_ROOT})
endif (NOT TARGET TeensyCore AND NOT ${CMAKE_SOURCE_DIR} MATCHES "CMakeTmp")


function(add_firmware_targets TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE "${TEENSY_ROOT}")

    # Generate the hex firmware files that can be flashed to the MCU.
    set(EEPROM_OPTS -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0)
    set(HEX_OPTS -O ihex -R .eeprom)

    if(NOT (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows"))
        set(PROCESS_SIZE_CMD_OUTPUT | tail -1 | xargs bash -c [[ TEXT=$0\; DATA=$1\; BSS=$2\; TOTAL_FLASH=262144\; TOTAL_RAM=65536\; FLASH=$((TEXT+DATA))\; RAM=$((DATA+BSS))\; echo "FLASH: $FLASH ($((FLASH*100/TOTAL_FLASH))%), RAM: $RAM ($((RAM*100/TOTAL_RAM))%), Free RAM: $((TOTAL_RAM-RAM))" ]])
    endif()

    add_custom_target(${TARGET_NAME}_Firmware ALL
            COMMENT "Firmware size:"
            COMMAND ${CMAKE_PRINT_SIZE} $<TARGET_FILE_NAME:${TARGET_NAME}> ${PROCESS_SIZE_CMD_OUTPUT}
            COMMAND ${CMAKE_OBJCOPY} ${EEPROM_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.eep
            COMMAND ${CMAKE_OBJCOPY} ${HEX_OPTS} $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.hex
            VERBATIM)

    add_custom_target(${TARGET_NAME}_Assembler
            COMMAND ${CMAKE_OBJDUMP} --demangle --disassemble --headers --wide $<TARGET_FILE_NAME:${TARGET_NAME}> > ${CMAKE_SOURCE_DIR}/build/source.S
            COMMAND ${CMAKE_OBJDUMP} --demangle --disassemble --source --wide $<TARGET_FILE_NAME:${TARGET_NAME}> > ${CMAKE_SOURCE_DIR}/build/source-with-text.S
            COMMAND ${CMAKE_OBJDUMP} --demangle --syms $<TARGET_FILE_NAME:${TARGET_NAME}> | sort > ${CMAKE_SOURCE_DIR}/build/source-symbols.txt
    )

    # See https://github.com/Koromix/ty
    find_file(TY_EXECUTABLE tyc 
        PATHS "/usr/local/bin" 
        DOC "Path to the 'tyc' executable that can upload programs to the Teensy")

    if(TY_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_Upload
                COMMAND "${TY_EXECUTABLE}" upload ${TARGET_NAME}.hex)
        add_dependencies(${TARGET_NAME}_Upload ${TARGET_NAME}_Firmware)
    endif()
endfunction(add_firmware_targets)


function(import_cmsis_dsp_library TARGET_NAME)
    target_include_directories(${TARGET_NAME} BEFORE PRIVATE "${CMSIS_ROOT}/Include")  # Make it a priority over old-version arm_math.h in teensy.
    target_link_libraries(${TARGET_NAME} ${CMSIS_ROOT}/Lib/GCC/libarm_cortexM4l_math.a)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DARM_MATH_CM4)
endfunction(import_cmsis_dsp_library)

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
