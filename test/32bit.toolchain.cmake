set(CMAKE_SYSTEM_NAME Linux)

# Compile as 32 bit code
add_compile_options("-m32")
link_libraries("-m32")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wno-deprecated-register" "-Wno-unknown-attributes")
endif()

