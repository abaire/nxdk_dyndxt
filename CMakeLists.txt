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

function(make_import_lib target_name output def_file)
    add_custom_command(
            OUTPUT ${output}
            COMMAND
            ${CMAKE_DLLTOOL}
            -m i386
            -d ${def_file}
            -l ${output}
            DEPENDS ${def_file}
            VERBATIM
    )
    add_custom_target(
            ${target_name}
            DEPENDS
            ${output}
    )
endfunction()

make_import_lib(
        xbdm_lib
        ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xbdm.dll.def
)


# Loader executable ------------------------------------

set(TARGET dynamic_dxt_loader)

add_library(
        ${TARGET}
        SHARED
        dll_loader/dll_loader.c
        dll_loader/dll_loader.h
        src/command_processor_util.c
        src/command_processor_util.h
        src/dxtmain.c
        src/module_registry.c
        src/module_registry.h
        src/nxdk_dxt_dll_main.h
        src/response_util.c
        src/response_util.h
        src/util.c
        src/util.h
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
        dll_loader
        src
        ${NXDK_DIR}
)
target_link_options(
        ${TARGET}
        PUBLIC
        -Wl,-machine:i386
        -B ${NXDK_DIR}/bin
        -Wl,-entry:DXTMainCRTStartup@0
        -Wl,-dynamicbase
        -Wl,-fixed:no
        -Wl,-def:${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET}.dll.def
        -v
)
target_link_libraries(
        ${TARGET}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
)
add_dependencies(
        ${TARGET}
        xbdm_lib
        bootstrap_l1
)

install(
        TARGETS
        ${TARGET}
        DESTINATION
        lib
)
install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/bootstrap_l1.asm.obj
        ${CMAKE_CURRENT_BINARY_DIR}/libdynamic_dxt_loader.lib
        ${CMAKE_CURRENT_BINARY_DIR}/xbdm.lib
        DESTINATION
        lib
)
install(
        FILES
        src/command_processor_util.h
        src/module_registry.h
        src/nxdk_dxt_dll_main.h
        src/xbdm.h
        src/xbdm_err.h
        DESTINATION
        include
)
