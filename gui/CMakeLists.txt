cmake_minimum_required(VERSION 3.16)

project(COINAlgebraGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automoc for Qt meta-object code generation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets QUIET)

if(NOT Qt5Widgets_FOUND AND NOT TARGET Qt5::Widgets)
    message(STATUS "Qt5 Widgets not found - GUI target will be skipped.")
    return()
endif()

set(COINALGEBRA_GUI_SOURCES
    main.cpp
    MainWindow.cpp
    QuiverView.cpp
)

# Build a small static core library from the COINAlgebra sources that do not
# depend on ROOT. This lets the GUI be built without ROOT.
add_library(COINAlgebraCore STATIC
    ${CMAKE_SOURCE_DIR}/src/Commands.cxx
    ${CMAKE_SOURCE_DIR}/src/PathAlgebra.cxx
    ${CMAKE_SOURCE_DIR}/src/PathProjectors.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayLevel.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayTransition.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayQuiver.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayPath.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayVector.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayProbability.cxx
    ${CMAKE_SOURCE_DIR}/src/RadioactiveDecayReader.cxx
    ${CMAKE_SOURCE_DIR}/src/PhotonEvaporationReader.cxx
    ${CMAKE_SOURCE_DIR}/src/DecayQuiverBuilder.cxx
)

target_include_directories(COINAlgebraCore PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(COINAlgebraCore PUBLIC cxx_std_17)

add_executable(coinalg_gui ${COINALGEBRA_GUI_SOURCES})
target_include_directories(coinalg_gui PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(coinalg_gui PRIVATE COINAlgebraCore Qt5::Widgets)

set_target_properties(coinalg_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
