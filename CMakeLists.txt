cmake_minimum_required(VERSION 3.18)
project(dyndxt_loader)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

enable_testing()
set(CMAKE_CXX_STANDARD 17)
include(CMakePushCheckState)
set(_CMAKE_PROCESSING_LANGUAGE "C")
include(CheckSymbolExists)
include(FindPkgConfig)

set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(GENERATED_FILES_DIR "${CMAKE_BINARY_DIR}/generated")

# Link libraries ------------------------------------
if (NOT CMAKE_DLLTOOL)
    FIND_PROGRAM(CMAKE_DLLTOOL dlltool CMAKE_FIND_ROOT_PATH_BOTH)
    if (NOT CMAKE_DLLTOOL)
        FIND_PROGRAM(CMAKE_DLLTOOL llvm-dlltool CMAKE_FIND_ROOT_PATH_BOTH)
    ENDIF ()
    if (NOT CMAKE_DLLTOOL)
        MESSAGE(FATAL_ERROR "Could not find dlltool command")
    ENDIF ()
ENDIF ()

add_custom_command(
        OUTPUT "${GENERATED_FILES_DIR}/xbdm.lib"
        COMMAND ${CMAKE_DLLTOOL}
            -m i386
            -d ${CMAKE_CURRENT_SOURCE_DIR}/src/xbdm.dll.def
            -l "${GENERATED_FILES_DIR}/xbdm.lib"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/xbdm.dll.def
        VERBATIM
)
add_custom_target(
        generate_xbdm_lib
        DEPENDS "${GENERATED_FILES_DIR}/xbdm.lib"
)
add_library(xbdm_lib INTERFACE)
add_dependencies(xbdm_lib generate_xbdm_lib)

# Loader executable ------------------------------------

set(TARGET dynamic_dxt_loader)

add_library(
        ${TARGET}
        src/dllmain.c
        src/xbdm.h
)

target_include_directories(
        ${TARGET}
        PRIVATE
        src
        ${NXDK_DIR}
)

target_link_options(
        ${TARGET}
        PUBLIC
        -m32
        -nostdlib
)
target_link_libraries(
        ${TARGET}
        xbdm_lib
)

