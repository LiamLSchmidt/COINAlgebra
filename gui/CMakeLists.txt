find_package(Qt5 COMPONENTS Widgets QUIET)
if(NOT TARGET Qt5::Widgets)
    message(STATUS "Qt5 Widgets not found - GUI target will be skipped.")
    return()
endif()

add_library(COINAlgebraGuiWidgets STATIC
    MainWindow.cpp MainWindow.h QuiverView.cpp QuiverView.h
    QuiverJson.cpp QuiverJson.h TransitionEditor.cpp TransitionEditor.h studio.qrc)
set_target_properties(COINAlgebraGuiWidgets PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_include_directories(COINAlgebraGuiWidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(COINAlgebraGuiWidgets PUBLIC COINAlgebra::Core Qt5::Widgets)

add_executable(DQStudio main.cpp)
target_link_libraries(DQStudio PRIVATE COINAlgebraGuiWidgets)
# Build the Studio launcher in gui/bin and install it as DQStudio.
set_target_properties(DQStudio PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
install(TARGETS DQStudio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
