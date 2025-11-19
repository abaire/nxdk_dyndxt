cmake_minimum_required(VERSION 3.18)
project(dyndxt_loader_tests)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Og")
set(CMAKE_CXX_FLAGS "-g -Og")

enable_testing()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
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
        test_util/xbdm_stubs.cpp
        test_util/xbdm_stubs.h
        test_util/windows.h
        ../src/command_processor_util.c
        ../src/command_processor_util.h
        ../src/xbdm.h
        third_party/nxdk/winapi/winnt.h
        third_party/nxdk/xboxkrnl/xboxdef.h
)
target_include_directories(
        command_processor_util_tests
        PRIVATE ../src
        PRIVATE test_util
        PRIVATE third_party/nxdk
)
target_link_libraries(
        command_processor_util_tests
        LINK_PRIVATE
        ${Boost_LIBRARIES}
)
add_test(NAME command_processor_util_tests COMMAND command_processor_util_tests)


# dll_loader_tests
add_executable(
        dll_loader_tests
        dll_loader/golden_dll.h
        dll_loader/relocated_B00D7000.h
        dll_loader/test_main.cpp
        ../dll_loader/dll_loader.c
        ../dll_loader/dll_loader.h
        third_party/nxdk/winapi/winnt.h
        third_party/nxdk/xboxkrnl/xboxdef.h
)
target_include_directories(
        dll_loader_tests
        PRIVATE ../dll_loader
        PRIVATE dll_loader
        PRIVATE third_party/nxdk
)
target_link_libraries(
        dll_loader_tests
        LINK_PRIVATE
        ${Boost_LIBRARIES}
)
add_test(NAME dll_loader_tests COMMAND dll_loader_tests)


# module_registry_tests
add_executable(
        module_registry_tests
        module_registry/test_main.cpp
        test_util/xbdm_stubs.cpp
        test_util/xbdm_stubs.h
        test_util/windows.h
        ../src/module_registry.c
        ../src/module_registry.h
        ../src/util.c
        ../src/util.h
        ../src/xbdm.h
        third_party/nxdk/winapi/winnt.h
        third_party/nxdk/xboxkrnl/xboxdef.h
)
target_include_directories(
        module_registry_tests
        PRIVATE ../src
        PRIVATE module_registry
        PRIVATE test_util
        PRIVATE third_party/nxdk
)
target_link_libraries(
        module_registry_tests
        LINK_PRIVATE
        ${Boost_LIBRARIES}
)
add_test(NAME module_registry_tests COMMAND module_registry_tests)
