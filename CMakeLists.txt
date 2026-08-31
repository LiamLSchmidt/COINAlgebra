cmake_minimum_required(VERSION 3.16)

project(COINAlgebra
    VERSION 0.1.0
    LANGUAGES CXX
)

#
# ============================================================
# Project configuration
# ============================================================
#

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# COINAlgebra directory layout
#

set(COINALGEBRA_SOURCE_DIR
    ${PROJECT_SOURCE_DIR}
)

set(COINALGEBRA_INCLUDE_DIR
    ${PROJECT_SOURCE_DIR}/include
)

set(COINALGEBRA_CONFIG_DIR
    ${PROJECT_SOURCE_DIR}/config
)

set(COINALGEBRA_LIBRARY_DIR
    ${PROJECT_SOURCE_DIR}/lib
)

set(COINALGEBRA_BINARY_DIR
    ${PROJECT_SOURCE_DIR}/bin
)

#
# ============================================================
# Installation directories
# ============================================================
#

include(GNUInstallDirs)

#
# For the development tree we use the repository itself
# as the installation prefix.
#

set(CMAKE_INSTALL_PREFIX
    ${PROJECT_SOURCE_DIR}
    CACHE PATH
    "COINAlgebra installation prefix"
)

set(CMAKE_INSTALL_LIBDIR
    lib
)

set(CMAKE_INSTALL_BINDIR
    bin
)

set(CMAKE_INSTALL_INCLUDEDIR
    include
)

#
# ============================================================
# ROOT
# ============================================================
#

find_package(ROOT REQUIRED COMPONENTS
    Core
    RIO
    Hist
    Rint
)

#
# Use ROOT's CMake configuration
#

include(${ROOT_USE_FILE})

#
# ============================================================
# Include directories
# ============================================================
#

include_directories(
    ${COINALGEBRA_INCLUDE_DIR}
)

#
# ============================================================
# COINAlgebra source files
# ============================================================
#

set(COINALGEBRA_SOURCES
  
    ${PROJECT_SOURCE_DIR}/src/COINAlgebra.cxx
    ${PROJECT_SOURCE_DIR}/src/Commands.cxx
    ${PROJECT_SOURCE_DIR}/src/DecayLevel.cxx
    ${PROJECT_SOURCE_DIR}/src/DecayTransition.cxx
    ${PROJECT_SOURCE_DIR}/src/DecayQuiver.cxx
    ${PROJECT_SOURCE_DIR}/src/DecayPath.cxx
    ${PROJECT_SOURCE_DIR}/src/DecayVector.cxx
)

#
# ============================================================
# COINAlgebra library
# ============================================================
#

add_library(COINAlgebra SHARED
    ${COINALGEBRA_SOURCES}
)

#
# Header include directory
#

target_include_directories(COINAlgebra
    PUBLIC
        ${COINALGEBRA_INCLUDE_DIR}
)

#
# ROOT libraries required by COINAlgebra
#

target_link_libraries(COINAlgebra
    PUBLIC
        ROOT::Core
        ROOT::RIO
        ROOT::Hist
)

#
# Put the shared library directly into lib/
#

set_target_properties(COINAlgebra PROPERTIES

    LIBRARY_OUTPUT_DIRECTORY
        ${COINALGEBRA_LIBRARY_DIR}

    BUILD_RPATH
        ${COINALGEBRA_LIBRARY_DIR}

    INSTALL_RPATH
        "$ORIGIN"

)

#
# ============================================================
# ROOT dictionary
# ============================================================
#

set(COINALGEBRA_HEADERS
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/COINAlgebra.h 
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/Commands.h	
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/DecayLevel.h
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/DecayTransition.h
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/DecayQuiver.h
    ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/DecayVector.h
)

#
# Generate the ROOT dictionary for the COINAlgebra library.
#
# NOINSTALL is important here because ROOT 6.26's
# ROOT_GENERATE_DICTIONARY() otherwise attempts to perform
# its own install() operation.
#

ROOT_GENERATE_DICTIONARY(
    COINAlgebraDict

    ${COINALGEBRA_HEADERS}

    LINKDEF
        ${COINALGEBRA_INCLUDE_DIR}/COINAlgebra/LinkDef.h

    MODULE
        COINAlgebra

    NOINSTALL
)

#
# ============================================================
# Copy ROOT dictionary artifacts into lib/
# ============================================================
#
# ROOT_GENERATE_DICTIONARY() creates the dictionary artifacts
# in the CMake binary directory (normally build/).
#
# We leave the generated files there and copy the two runtime
# artifacts into the repository's lib/ directory:
#
#   libCOINAlgebra_rdict.pcm
#   libCOINAlgebra.rootmap
#
# This makes the development tree self-contained:
#
#   lib/
#       libCOINAlgebra.so
#       libCOINAlgebra_rdict.pcm
#       libCOINAlgebra.rootmap
#
# ============================================================
#

set(COINALGEBRA_PCM
    ${CMAKE_CURRENT_BINARY_DIR}/libCOINAlgebra_rdict.pcm
)

set(COINALGEBRA_ROOTMAP
    ${CMAKE_CURRENT_BINARY_DIR}/libCOINAlgebra.rootmap
)

add_custom_command(
    TARGET COINAlgebra
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${COINALGEBRA_PCM}
        ${COINALGEBRA_LIBRARY_DIR}/libCOINAlgebra_rdict.pcm

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${COINALGEBRA_ROOTMAP}
        ${COINALGEBRA_LIBRARY_DIR}/libCOINAlgebra.rootmap

    COMMENT
        "Copying ROOT dictionary files to lib/"
)

#
# ============================================================
# COINAlgebra executable
# ============================================================
#

add_executable(coinalgebra

    ${PROJECT_SOURCE_DIR}/src/main.cxx

)

#
# Link the executable against the COINAlgebra library and
# ROOT's interactive interpreter.
#

target_link_libraries(coinalgebra
    PRIVATE
        COINAlgebra
        ROOT::Rint
)

#
# Include COINAlgebra headers for the executable
#

target_include_directories(coinalgebra
    PRIVATE
        ${COINALGEBRA_INCLUDE_DIR}
)

#
# Put executable directly into bin/
#

set_target_properties(coinalgebra PROPERTIES

    RUNTIME_OUTPUT_DIRECTORY
        ${COINALGEBRA_BINARY_DIR}

    BUILD_RPATH
        ${COINALGEBRA_LIBRARY_DIR}

    INSTALL_RPATH
        "$ORIGIN/../lib"

)

#
# ============================================================
# Installation
# ============================================================
#

#
# Install the main library
#

install(
    TARGETS COINAlgebra
    LIBRARY DESTINATION lib
)

#
# Install the executable
#

install(
    TARGETS coinalgebra
    RUNTIME DESTINATION bin
)

#
# Install public headers
#

install(
    DIRECTORY
        ${COINALGEBRA_INCLUDE_DIR}/
    DESTINATION
        include
)

#
# Install configuration/startup files
#

install(
    DIRECTORY
        ${COINALGEBRA_CONFIG_DIR}/
    DESTINATION
        config
)

#
# Install ROOT dictionary artifacts
#
# These files have already been copied into lib/ during the
# build. OPTIONAL prevents installation from failing if the
# dictionary has not yet been generated.
#

install(
    FILES
        ${COINALGEBRA_LIBRARY_DIR}/libCOINAlgebra_rdict.pcm
        ${COINALGEBRA_LIBRARY_DIR}/libCOINAlgebra.rootmap
    DESTINATION
        lib
    OPTIONAL
)

#
# ============================================================
# Summary
# ============================================================
#

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "           COINAlgebra Configuration")
message(STATUS "==============================================")
message(STATUS "Version:       ${PROJECT_VERSION}")
message(STATUS "Source:        ${PROJECT_SOURCE_DIR}")
message(STATUS "Headers:       ${COINALGEBRA_INCLUDE_DIR}")
message(STATUS "Library:       ${COINALGEBRA_LIBRARY_DIR}")
message(STATUS "Executable:    ${COINALGEBRA_BINARY_DIR}")
message(STATUS "Config:        ${COINALGEBRA_CONFIG_DIR}")
message(STATUS "ROOT:          ${ROOT_VERSION}")
message(STATUS "C++ Standard:  C++${CMAKE_CXX_STANDARD}")
message(STATUS "==============================================")
message(STATUS "")
