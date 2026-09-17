cmake_minimum_required(VERSION 3.24)

project(CCOS
    VERSION 0.1.0
    DESCRIPTION "CCOS - Open Source Professional Video Editor"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CCOS_BUILD_TESTS "Build unit tests" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

add_library(ccos_core
    src/core/Time.cpp
    src/core/Uuid.cpp
    src/core/CommandStack.cpp
    src/project/Project.cpp
    src/project/ProjectSerializer.cpp
    src/media/MediaAsset.cpp
    src/media/MediaImporter.cpp
    src/media/MediaProbe.cpp
    src/timeline/Clip.cpp
    src/timeline/Track.cpp
    src/timeline/Timeline.cpp
    src/timeline/TimelineEditor.cpp
    src/timeline/Keyframe.cpp
    src/render/RenderManager.cpp
    src/render/FfmpegExporter.cpp
)

target_include_directories(ccos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(ccos_core PUBLIC cxx_std_20)
target_compile_definitions(ccos_core PUBLIC QT_NO_KEYWORDS)
target_link_libraries(ccos_core PUBLIC Qt6::Core)

if(MSVC)
    target_compile_options(ccos_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(ccos_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(ccos_editor
    src/app/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.hpp
)

target_link_libraries(ccos_editor PRIVATE ccos_core Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)
target_include_directories(ccos_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(ccos_editor PRIVATE /W4 /permissive-)
else()
    target_compile_options(ccos_editor PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

install(TARGETS ccos_editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)

if(CCOS_BUILD_TESTS)
    enable_testing()
    find_package(GTest 1.14 REQUIRED)
    add_executable(ccos_tests
        tests/core/TimeTests.cpp
        tests/core/CommandStackTests.cpp
        tests/project/ProjectTests.cpp
        tests/timeline/TimelineTests.cpp
        tests/timeline/TimelineEditorTests.cpp
        tests/timeline/KeyframeTests.cpp
    )
    target_link_libraries(ccos_tests PRIVATE ccos_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(ccos_tests)
endif()

include(GNUInstallDirs)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR} RENAME LICENSE.txt)
