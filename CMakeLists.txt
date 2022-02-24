cmake_minimum_required(VERSION 3.18)
project(dyndxt_loader)

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_SHARED_LIBRARY_SUFFIX_C ".dll")
set(CMAKE_SHARED_LIBRARY_SUFFIX_CXX ".dll")

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(CMAKE_CXX_STANDARD 17)
include(CMakePushCheckState)
set(_CMAKE_PROCESSING_LANGUAGE "C")
include(CheckSymbolExists)
include (ExternalProject)
include(FindPkgConfig)

set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(bootstrap)

set(ddxt_bin_dir ${CMAKE_CURRENT_BINARY_DIR}/ddxt-bin)
ExternalProject_Add(
        dyndxt_loader_tests
        PREFIX dyndxt_loader_tests
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test
        BINARY_DIR dyndxt_loader_tests
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL FALSE
        BUILD_ALWAYS TRUE
)


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
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
        COMMAND ${CMAKE_DLLTOOL}
            -m i386
            -d ${CMAKE_CURRENT_SOURCE_DIR}/src/xbdm.dll.def
            -l "${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/xbdm.dll.def
        VERBATIM
)
add_custom_target(
        generate_xbdm_lib
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
)

# Loader executable ------------------------------------

set(TARGET dynamic_dxt_loader)

add_library(
        ${TARGET}
        SHARED
        src/command_processor_util.c
        src/command_processor_util.h
        src/dxtmain.c
        src/response_util.c
        src/response_util.h
        src/xbdm.h
        src/xbdm_err.h
)
target_compile_options(
        ${TARGET}
        PRIVATE
        -D DLLEXPORT
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
        -Wl,-machine:i386
        -B ${NXDK_DIR}/bin
        -Wl,-entry:DxtMain
        -Wl,-dynamicbase
        -Wl,-fixed:no
        -v
)
target_link_libraries(
        ${TARGET}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
)

# Force creation of the bootstrap objects, even though they are not actually
# depended on by the dynamic_dxt_loader itself.
add_dependencies(${TARGET} generate_xbdm_lib bootstrap_l1 bootstrap_l2_thunk)


install(
        TARGETS
        ${TARGET}
        DESTINATION
        lib
)
install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
        DESTINATION
        lib
)
install(
        FILES
        src/command_processor_util.h
        DESTINATION
        include
)
