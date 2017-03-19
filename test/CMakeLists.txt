
set(TEST_SOURCE_FILES
        main_test.cpp
        platform_mocks.cpp
        test_pulse_processor.cpp
)

# Compile CMSIS as a library.
set(CMSIS_ROOT "${CMAKE_SOURCE_DIR}/libs/CMSIS/CMSIS" CACHE PATH "Path to the CMSIS root directory")
file(GLOB_RECURSE CMSIS_CORE_FILES "${CMSIS_ROOT}/DSP_Lib/Source/*_f32.c")
set(CMSIS_CORE_FILES "${CMSIS_CORE_FILES}" "${CMSIS_ROOT}/DSP_Lib/Source/CommonTables/arm_common_tables.c")
add_library(cmsis STATIC EXCLUDE_FROM_ALL "${CMSIS_CORE_FILES}")
target_compile_definitions(cmsis PUBLIC "ARM_MATH_CM4")
target_include_directories(cmsis PUBLIC "${CMSIS_ROOT}/Include")

# We have only one test executable
add_executable(main-test "${TEST_SOURCE_FILES}")
target_include_directories(main-test PUBLIC "../libs/Catch")
target_link_libraries(main-test sensor-core)

add_test(NAME test COMMAND main-test)
