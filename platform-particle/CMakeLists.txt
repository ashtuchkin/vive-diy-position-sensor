
set(PARTICLE_PLATFORM "photon" CACHE STRING "Particle platform to compile for")
set(FIRMWARE_DIR "${CMAKE_SOURCE_DIR}/libs/particle-firmware")
set(APPDIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(TARGET_DIR "${CMAKE_BINARY_DIR}/particle")
set(TARGET_FILE "${PARTICLE_PLATFORM}")
set(MAKE_FLAGS 
        PLATFORM=${PARTICLE_PLATFORM} MODULAR=n PLATFORM_THREADING=0
        APPDIR=${APPDIR} TARGET_DIR=${TARGET_DIR} TARGET_FILE=${TARGET_FILE}
)
add_definitions(-DPLATFORM_THREADING=0)
add_definitions(-DPLATFORM_NAME=${PARTICLE_PLATFORM})
if (${PARTICLE_PLATFORM} STREQUAL "photon")
    add_definitions(-DPLATFORM_ID=6 -DPRODUCT_ID=6)
else ()
    # Please define PLATFORM_ID and PRODUCT_ID according to the platform name.
    message(FATAL_ERROR "Unsupported particle platform: ${PARTICLE_PLATFORM}")
endif ()

# Download firmware and make deps.
include(ExternalProject)
ExternalProject_Add(
    particle-firmware-deps
    EXCLUDE_FROM_ALL 1
    SOURCE_DIR "${FIRMWARE_DIR}"
    GIT_REPOSITORY "https://github.com/ashtuchkin/spark-firmware.git"
    GIT_TAG "enable-exceptions-and-cpp14"
    CONFIGURE_COMMAND ""
    BINARY_DIR "${FIRMWARE_DIR}/main"
    BUILD_COMMAND make build_dependencies -j1 ${MAKE_FLAGS}
    INSTALL_COMMAND ""
)

# CMSIS library
set(CMSIS_ROOT "${CMAKE_SOURCE_DIR}/libs/CMSIS/CMSIS" CACHE PATH "Path to the CMSIS root directory")
add_library(cmsis STATIC IMPORTED GLOBAL)
set_target_properties(
    cmsis PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMSIS_ROOT}/Include"
    INTERFACE_COMPILE_DEFINITIONS "ARM_MATH_CM3"
    IMPORTED_LOCATION "${CMSIS_ROOT}/Lib/GCC/libarm_cortexM3l_math.a"
)

# Build full firmware  particle-firmware-deps 
add_custom_target(sensor-firmware ALL
    DEPENDS sensor-core
    WORKING_DIRECTORY "${FIRMWARE_DIR}/main"
    COMMAND make all ${MAKE_FLAGS} ADDITIONAL_LIB_DIRS='$<TARGET_FILE_DIR:sensor-core> $<TARGET_FILE_DIR:cmsis>' ADDITIONAL_LIBS='sensor-core arm_cortexM3l_math'
)

add_custom_target(sensor-firmware-clean
    DEPENDS particle-firmware-deps
    WORKING_DIRECTORY "${FIRMWARE_DIR}/main"
    COMMAND make clean ${MAKE_FLAGS}
)

add_custom_target(sensor-firmware-upload
    DEPENDS sensor-firmware
    COMMAND particle flash --usb ${TARGET_DIR}/${TARGET_FILE}.bin
)

add_custom_target(sensor-firmware-assembler
    DEPENDS sensor-firmware
    COMMAND ${CMAKE_OBJDUMP} --demangle --disassemble --source --wide ${TARGET_DIR}/${TARGET_FILE}.elf > ${TARGET_DIR}/${TARGET_FILE}-source.txt
)