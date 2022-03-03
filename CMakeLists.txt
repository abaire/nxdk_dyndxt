cmake_minimum_required(VERSION 3.18)
project(DynDXT_Superbuild NONE)

include(ExternalProject)

set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

ExternalProject_Add(
        external_nxdk
        PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
        URL https://github.com/XboxDev/nxdk/tarball/master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} -E env
            NXDK_CFLAGS=-O2
            NXDK_CXXFLAGS=-O2
            <SOURCE_DIR>/bin/activate make -C <SOURCE_DIR> LTO=y V=1 NXDK_ONLY=y
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_BYPRODUCTS
        ${nxdk_libs}
)
set_target_properties(external_nxdk PROPERTIES EXCLUDE_FROM_ALL FALSE)

ExternalProject_Get_property(external_nxdk SOURCE_DIR)
set(external_nxdk_source_dir ${SOURCE_DIR})

#ExternalProject_Add(
#        dyndxt_loader
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
#        CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${external_nxdk_source_dir}/share/toolchain-nxdk.cmake
#        INSTALL_COMMAND
#        cmake --build . --target install --config Release
#        BUILD_BYPRODUCTS
#)
#set_target_properties(
#        dyndxt_loader
#        PROPERTIES
#        EXCLUDE_FROM_ALL FALSE
#)
#add_dependencies(dyndxt_loader external_all)

#
#link_directories(
#        ${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
#)
#set(
#        ADDITIONAL_INCLUDE_DIRS
#        $<BUILD_INTERFACE:${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
#)
#
#find_library(
#        LIB_NXDK
#        NAMES
#        winapi
#        HINTS
#        ${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
#)
