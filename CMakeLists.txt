cmake_minimum_required(VERSION 3.18)
project(xbdm_gdb_bridge)

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

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/build_util/linker_script.ld)
target_link_options(
        ${TARGET}
        PUBLIC
        -m32
        -nostdlib
        $<$<C_COMPILER_ID:GNU>:-Wl,--script=${LINKER_SCRIPT}>
        $<$<C_COMPILER_ID:Clang>:-arch i386 -T ${LINKER_SCRIPT}>
)
target_link_libraries(
        ${TARGET}
        xbdm_lib
)
set_target_properties(${TARGET} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})



#if (NOT CMAKE_DLLTOOL)
#    FIND_PROGRAM(CMAKE_DLLTOOL dlltool CMAKE_FIND_ROOT_PATH_BOTH)
#    if (NOT CMAKE_DLLTOOL)
#        FIND_PROGRAM(CMAKE_DLLTOOL llvm-dlltool CMAKE_FIND_ROOT_PATH_BOTH)
#    ENDIF ()
#    if (NOT CMAKE_DLLTOOL)
#        MESSAGE(FATAL_ERROR "Could not find dlltool command")
#    ENDIF ()
#ENDIF ()
#
#add_custom_command(
#        OUTPUT "${GENERATED_FILES_DIR}/xbdm.lib"
#        COMMAND ${CMAKE_DLLTOOL}
#            -m i386
#            -d ${CMAKE_CURRENT_SOURCE_DIR}/xbdm.dll.def
#            -l "${GENERATED_FILES_DIR}/xbdm.lib"
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xbdm.dll.def
#        VERBATIM
#)
#add_custom_target(
#        generate_xbdm_lib
#        DEPENDS "${GENERATED_FILES_DIR}/xbdm.lib"
#)
#add_library(xbdm_lib INTERFACE)
#add_dependencies(xbdm_lib generate_xbdm_lib)
#
#
#set(TARGET bootstrap_l2)
#add_executable(
#        ${TARGET}
#        WIN32
#        EXCLUDE_FROM_ALL
#        bootstrap_l2.c
#        bootstrap_l2_imports.h
#        windefs.h
#        xbdm.h
#)
#
#target_compile_options(
#        ${TARGET}
#        PRIVATE
#        --target=i386-pc-none-gnu
#        -mcpu=i386
#        -std=c99
#        -m32
#        $<$<C_COMPILER_ID:GNU>:-nostartfiles -pie>
#        -nodefaultlibs
#        -ffreestanding
#        -fno-builtin
#        -fpie
#        -Os
#)
#
#set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/binary.ld)
#target_link_options(
#        ${TARGET}
#        PUBLIC
#        -m32
#        -nostdlib
#        $<$<C_COMPILER_ID:GNU>:-Wl,--script=${LINKER_SCRIPT}>
#        $<$<C_COMPILER_ID:Clang>:-arch i386 -T ${LINKER_SCRIPT}>
#)
#target_link_libraries(
#        ${TARGET}
#        xbdm_lib
#)
#set_target_properties(${TARGET} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
