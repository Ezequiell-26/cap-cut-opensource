cmake_minimum_required(VERSION 3.24)

project(CCOS
    VERSION 0.2.0
    DESCRIPTION "CCOS - Open Source Professional Video Editor"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CCOS_BUILD_TESTS "Build unit tests" ON)
option(CCOS_FETCH_GTEST "Fetch GoogleTest when it is not installed" ON)

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
    src/media/MediaCache.cpp
    src/media/ProxyManager.cpp
    src/media/MediaDerivatives.cpp
    src/timeline/Clip.cpp
    src/timeline/Track.cpp
    src/timeline/Timeline.cpp
    src/timeline/TimelineEditor.cpp
    src/timeline/Keyframe.cpp
    src/audio/AudioMixer.cpp
    src/effects/BuiltinEffects.cpp
    src/transitions/Transition.cpp
    src/text/SubtitleParser.cpp
    src/ai/WhisperAdapter.cpp
    src/plugins/PluginManager.cpp
    src/render/RenderManager.cpp
    src/render/FfmpegExporter.cpp
    src/render/TimelineExporter.cpp
    src/render/TimelineCompositor.cpp
    src/render/HardwareCapabilities.cpp
    src/render/RenderExecutor.cpp
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

include(GNUInstallDirs)
install(TARGETS ccos_editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)

set(CPACK_PACKAGE_NAME "CCOS")
set(CPACK_PACKAGE_VENDOR "CCOS Open Source")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open source C++ video editor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "CCOS maintainers")
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)

if(CCOS_BUILD_TESTS)
    enable_testing()
    find_package(GTest 1.14 QUIET)
    if(NOT GTest_FOUND AND CCOS_FETCH_GTEST)
        include(FetchContent)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    if(NOT TARGET GTest::gtest_main)
        message(FATAL_ERROR "GoogleTest is required. Install GTest or configure with -DCCOS_FETCH_GTEST=ON.")
    endif()

    add_executable(ccos_tests
        tests/core/TimeTests.cpp
        tests/core/CommandStackTests.cpp
        tests/project/ProjectTests.cpp
        tests/timeline/TimelineTests.cpp
        tests/timeline/TimelineEditorTests.cpp
        tests/timeline/KeyframeTests.cpp
        tests/media/MediaCacheTests.cpp
    )
    target_link_libraries(ccos_tests PRIVATE ccos_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(ccos_tests)
endif()

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR} RENAME LICENSE.txt)
