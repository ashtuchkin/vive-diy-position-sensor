cmake_minimum_required(VERSION 3.5)

set(LOCAL_BUILD "FALSE" CACHE STRING "Build locally instead; for unit tests")

project(vive-diy-position-sensor)

if (NOT ${LOCAL_BUILD})

  set(CMAKE_TOOLCHAIN_FILE "teensy-arm.toolchain.cmake")
  set(CMAKE_DISABLE_SOURCE_CHANGES ON)

  set(SOURCE_FILES
          main.cpp
          setup.cpp
          data_decoder.cpp
          geometry.cpp
          mavlink.cpp
  )

  add_executable(vive-diy-position-sensor "${SOURCE_FILES}")
  add_firmware_targets(vive-diy-position-sensor)

  import_cmsis_dsp_library(vive-diy-position-sensor)
  import_arduino_library(vive-diy-position-sensor mavlink_v2)
else (NOT ${LOCAL_BUILD})

  add_executable(skyview_lighthouse_test
                 skyview_lighthouse.cc
                 skyview_lighthouse_test.cc)

  add_executable(ring_buffer_test
                 ring_buffer_test.cc)

endif (NOT ${LOCAL_BUILD})

