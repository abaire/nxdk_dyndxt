cmake_minimum_required(VERSION 3.18)
project(dyndxt_loader_tests)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Og")
set(CMAKE_CXX_FLAGS "-g -Og")

enable_testing()
find_package(
        Boost 1.70
        COMPONENTS
        unit_test_framework
        REQUIRED
)
include_directories("${Boost_INCLUDE_DIR}")

# Tests ----------------------------------------------

# command_processor_util_tests
add_executable(
        command_processor_util_tests
        command_processor_util/test_main.cpp
        ../src/command_processor_util.c
        ../src/command_processor_util.h
)
target_compile_options(
        command_processor_util_tests
        PRIVATE
        -D __NO_DLL_LINKAGE
)
target_include_directories(
        command_processor_util_tests
        PRIVATE ../src
        PRIVATE test
)
target_link_libraries(
        command_processor_util_tests
        LINK_PRIVATE
        ${Boost_LIBRARIES}
)
add_test(NAME command_processor_util_tests COMMAND command_processor_util_tests)
