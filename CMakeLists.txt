cmake_minimum_required(VERSION 3.16)
project(COINAlgebra VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_WITH_ROOT "Build the ROOT library, dictionary and interactive environment" ON)
option(BUILD_GUI "Build the Qt quiver builder when Qt5 Widgets is available" ON)
include(GNUInstallDirs)
include(CTest)
include(cmake/COINAlgebraSources.cmake)

# All mathematical/data code is compiled once without ROOT or Qt dependencies.
add_library(COINAlgebraObjects OBJECT ${COINALGEBRA_SOURCES})
set_target_properties(COINAlgebraObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(COINAlgebraObjects PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(COINAlgebraObjects PUBLIC cxx_std_17)

add_library(COINAlgebraCore STATIC $<TARGET_OBJECTS:COINAlgebraObjects>)
add_library(COINAlgebra::Core ALIAS COINAlgebraCore)
target_include_directories(COINAlgebraCore PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(COINAlgebraCore PUBLIC cxx_std_17)
set_target_properties(COINAlgebraCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
install(TARGETS COINAlgebraCore ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_WITH_ROOT)
    find_package(ROOT REQUIRED COMPONENTS Core RIO Hist Rint)
    add_library(COINAlgebra SHARED
        $<TARGET_OBJECTS:COINAlgebraObjects>
        src/ROOT/COINAlgebra.cxx src/ROOT/Commands.cxx)
    target_include_directories(COINAlgebra PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(COINAlgebra PUBLIC ROOT::Core ROOT::RIO ROOT::Hist PRIVATE ROOT::Rint)
    set_target_properties(COINAlgebra PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        BUILD_RPATH ${PROJECT_SOURCE_DIR}/lib
        INSTALL_RPATH "$ORIGIN")

    ROOT_GENERATE_DICTIONARY(COINAlgebraDict
        ${COINALGEBRA_HEADERS}
        ${PROJECT_SOURCE_DIR}/include/COINAlgebra/ROOT/COINAlgebra.h
        ${PROJECT_SOURCE_DIR}/include/COINAlgebra/ROOT/Commands.h
        LINKDEF ${PROJECT_SOURCE_DIR}/include/COINAlgebra/ROOT/LinkDef.h
        MODULE COINAlgebra NOINSTALL)

    add_custom_command(TARGET COINAlgebra POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/libCOINAlgebra_rdict.pcm
            ${PROJECT_SOURCE_DIR}/lib/libCOINAlgebra_rdict.pcm
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/libCOINAlgebra.rootmap
            ${PROJECT_SOURCE_DIR}/lib/libCOINAlgebra.rootmap
        COMMENT "Copying ROOT dictionary files to lib/")

    add_executable(coinalgebra apps/root/main.cxx)
    target_link_libraries(coinalgebra PRIVATE COINAlgebra ROOT::Rint)
    set_target_properties(coinalgebra PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
        BUILD_RPATH ${PROJECT_SOURCE_DIR}/lib
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    install(TARGETS COINAlgebra coinalgebra
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES
        ${PROJECT_SOURCE_DIR}/lib/libCOINAlgebra_rdict.pcm
        ${PROJECT_SOURCE_DIR}/lib/libCOINAlgebra.rootmap
        DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(BUILD_GUI)
    add_subdirectory(gui)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY config/ DESTINATION config)
