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


set(TEENSY_ROOT "${CMAKE_SOURCE_DIR}/libs/cores/teensy3" CACHE PATH "Path to the Teensy 'cores/teensy3' repository")

set(TARGET_FLAGS "-mcpu=cortex-m4 -mthumb -mfp16-format=ieee")
set(OPTIMIZE_FLAGS "-O2" CACHE STRING "Optimization flags")
set(CMAKE_C_FLAGS "${OPTIMIZE_FLAGS} -Wall -ffunction-sections -fdata-sections -Wstack-usage=256 ${TARGET_FLAGS}" CACHE STRING "C/C++ flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fsingle-precision-constant -Woverloaded-virtual" CACHE STRING "C++ flags")

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "" FORCE)  # Don't do -O3 because it increases the size. Just remove asserts.
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "" FORCE)

set(LINKER_FLAGS "--gc-sections,--relax,--defsym=__rtc_localtime=0" CACHE STRING "Ld flags")
set(CXX_LINKER_FLAGS "${OPTIMIZE_FLAGS} -Wl,${LINKER_FLAGS} ${TARGET_FLAGS} -T${TEENSY_ROOT}/mk20dx256.ld")

set(CMAKE_SHARED_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Shared Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Module Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${CXX_LINKER_FLAGS}" CACHE STRING "Executable Linker flags" FORCE)



