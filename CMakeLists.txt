cmake_minimum_required(VERSION 3.24)

project(CCOS
    VERSION 0.6.0
    DESCRIPTION "CCOS - Open Source Professional Video Editor with AI & ML"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CCOS_BUILD_TESTS "Build unit tests" ON)
option(CCOS_BUILD_CLI "Build headless CLI/API runner" ON)
option(CCOS_FETCH_GTEST "Fetch GoogleTest when it is not installed" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets Svg Concurrent Test)
find_package(OpenCV 4.8 REQUIRED COMPONENTS core imgproc video dnn imgcodecs)
find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

# Optional packages for enhanced features.
find_package(MediaPipe QUIET)
find_package(Skia QUIET)
find_package(RLottie QUIET)
find_package(NanoSVG QUIET)

add_library(ccos_core
    src/api/EditorApi.cpp
    src/api/StockMediaApi.cpp
    src/api/LibreTranslateApi.cpp
    src/api/ColorApi.cpp
    src/api/OpenMeteoApi.cpp
    src/api/AudioApi.cpp
    src/api/AssetsApi.cpp
    src/api/PremiumStockApi.cpp
    src/api/SocialMediaApi.cpp
    src/api/AccessibilityApi.cpp

    src/vision/ComputerVisionApi.cpp
    src/audio_pro/AudioProcessingApi.cpp
    src/graphics/GraphicsApi.cpp

    src/core/Time.cpp
    src/core/Uuid.cpp
    src/core/CommandStack.cpp
    src/core/ProcessRunner.cpp
    src/core/JobSystem.cpp

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
    src/ai/OpenAICompatibleProvider.cpp
    src/ai/AdvancedAI.cpp

    src/plugins/PluginManager.cpp

    src/render/RenderManager.cpp
    src/render/FfmpegExporter.cpp
    src/render/TimelineExporter.cpp
    src/render/TimelineCompositor.cpp
    src/render/TextComposer.cpp
    src/render/HardwareCapabilities.cpp
    src/render/RenderExecutor.cpp
)

target_include_directories(ccos_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}
)

target_compile_features(ccos_core PUBLIC cxx_std_20)
target_compile_definitions(ccos_core PUBLIC QT_NO_KEYWORDS)
target_link_libraries(ccos_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Svg
    Qt6::Concurrent
    ${OpenCV_LIBS}
    ZLIB::ZLIB
)

if(TARGET MediaPipe::mediapipe)
    target_link_libraries(ccos_core PUBLIC MediaPipe::mediapipe)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_MEDIAPIPE)
endif()

if(TARGET Skia::skia)
    target_link_libraries(ccos_core PUBLIC Skia::skia)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_SKIA)
endif()

if(TARGET RLottie::rlottie)
    target_link_libraries(ccos_core PUBLIC RLottie::rlottie)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_RLOTTIE)
endif()

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

if(CCOS_BUILD_CLI)
    add_executable(ccos_cli src/cli/main.cpp)
    target_link_libraries(ccos_cli PRIVATE ccos_core Qt6::Core)
    target_include_directories(ccos_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    install(TARGETS ccos_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS ccos_editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR} RENAME LICENSE.txt)

if(CCOS_BUILD_TESTS)
    enable_testing()

    find_package(GTest 1.14 QUIET)
    if(NOT GTest_FOUND AND CCOS_FETCH_GTEST)
        include(FetchContent)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
        FetchContent_MakeAvailable(googletest)
    endif()

    if(NOT TARGET GTest::gtest_main)
        message(FATAL_ERROR "GoogleTest is required. Install GTest or configure with -DCCOS_FETCH_GTEST=ON.")
    endif()

    add_executable(ccos_tests
        tests/core/TimeTests.cpp
        tests/core/CommandStackTests.cpp
        tests/project/ProjectTests.cpp
        tests/project/ProjectClipPropertiesTests.cpp
        tests/timeline/TimelineTests.cpp
        tests/timeline/TimelineEditorTests.cpp
        tests/timeline/KeyframeTests.cpp
        tests/media/MediaCacheTests.cpp
        tests/effects/BuiltinEffectsTests.cpp
        tests/text/SubtitleParserTests.cpp
        tests/unit/core/JobSystemTest.cpp
        tests/unit/core/ProcessRunnerTest.cpp
    )

    target_link_libraries(ccos_tests PRIVATE
        ccos_core
        GTest::gtest_main
        Qt6::Test
    )
    target_include_directories(ccos_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    if(MSVC)
        target_compile_options(ccos_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ccos_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()

    include(GoogleTest)
    gtest_discover_tests(ccos_tests)
endif()

set(CPACK_PACKAGE_NAME "CCOS")
set(CPACK_PACKAGE_VENDOR "CCOS Open Source")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open source C++ video editor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "CCOS maintainers")
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
