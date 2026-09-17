cmake_minimum_required(VERSION 3.24)

project(CCOS
    VERSION 0.7.0
    DESCRIPTION "CCOS - Open Source Professional Video Editor with AI & ML"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CCOS_BUILD_TESTS "Build unit tests" ON)
option(CCOS_BUILD_CLI "Build headless CLI/API runner" ON)
option(CCOS_FETCH_GTEST "Fetch GoogleTest when it is not installed" ON)
option(CCOS_ENABLE_MIT_FOUNDATION "Fetch the curated MIT-licensed C++ foundation stack" ON)
option(CCOS_ENABLE_MIT_MEDIA_3D "Fetch optional MIT C++ libraries for 3D/motion-graphics asset processing" OFF)
option(CCOS_ENABLE_MIT_TOOLING "Fetch optional MIT C++ tooling libraries for CLI/XML/test infrastructure" OFF)
option(CCOS_ENABLE_MINIAUDIO "Make miniaudio available to optional audio device backends" OFF)
option(CCOS_ENABLE_ONNXRUNTIME "Enable the optional ONNX Runtime integration" OFF)
option(CCOS_ENABLE_LOCAL_AUTOMATION_API "Build the loopback-only automation API for agents" ON)
option(CCOS_ENABLE_VISION "Build the optional OpenCV + MediaPipe vision subsystem" OFF)
option(CCOS_ENABLE_EXPERIMENTAL_PREVIEW "Build the unfinished preview prototype" OFF)
option(CCOS_ENABLE_RENDER_SCHEDULER "Build the legacy RenderQueue scheduler" OFF)
option(CCOS_ENABLE_OPENTIMELINEIO "Enable optional OpenTimelineIO C++ interchange support" OFF)
option(CCOS_ENABLE_OPENCOLORIO "Enable optional OpenColorIO color-management support" OFF)
option(CCOS_ENABLE_MIT_UI_EXTENSIONS "Enable Dear ImGui + ImGuizmo integration" OFF)
option(CCOS_ENABLE_MIT_DIAGNOSTICS "Enable cpptrace crash/stack diagnostics" OFF)
option(CCOS_ENABLE_MIT_STORAGE "Enable unordered_dense/date/foonathan memory helpers" OFF)
option(CCOS_ENABLE_PRO_TEXT "Use locally audited FreeType + HarfBuzz + libass" OFF)
option(CCOS_ENABLE_PRO_IMAGE_IO "Use locally audited OpenEXR + OpenImageIO + AVIF/WebP/JXL" OFF)
option(CCOS_ENABLE_PRO_AUDIO_IO "Use locally audited RtAudio + RtMidi + libsamplerate + SpeexDSP" OFF)
option(CCOS_ENABLE_PRO_CODECS "Use locally audited dav1d + SVT-AV1" OFF)
option(CCOS_ENABLE_PRO_STORAGE "Use locally audited zstd + lz4 + xxHash + libarchive + libzip" OFF)
option(CCOS_ENABLE_CULTURAL_MEDIA_API "Build open cultural-heritage media API adapters" ON)
option(CCOS_ENABLE_LLAMA_SERVER_PROFILE "Enable the local llama.cpp/llama-server provider profile" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCOSMitDependencies.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCOSProfessionalDependencies.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCOSExtendedOpenSource.cmake)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets Concurrent)

set(CCOS_SOURCES
    src/api/EditorApi.cpp
    src/api/StockMediaApi.cpp
    src/api/CreativeMediaApi.cpp
    src/api/NasaMediaApi.cpp
    src/api/CulturalMediaApi.cpp
    src/api/LibreTranslateApi.cpp
    src/api/ColorApi.cpp
    src/api/OpenMeteoApi.cpp
    src/api/AudioApi.cpp
    src/api/AssetsApi.cpp
    src/api/PremiumStockApi.cpp
    src/api/SocialMediaApi.cpp
    src/api/AccessibilityApi.cpp
    src/core/Time.cpp
    src/core/Uuid.cpp
    src/core/CommandStack.cpp
    src/core/ProcessRunner.cpp
    src/core/JobSystem.cpp
    src/core/AITooling.cpp
    src/core/PipelineGraph.cpp
    src/project/Project.cpp
    src/project/ProjectSerializer.cpp
    src/project/ProjectSchema.cpp
    src/project/ProjectRecovery.cpp
    src/media/MediaAsset.cpp
    src/media/MediaImporter.cpp
    src/media/MediaProbe.cpp
    src/media/MediaCache.cpp
    src/media/ProxyManager.cpp
    src/media/MediaDerivatives.cpp
    src/media/ThumbnailGenerator.cpp
    src/media/WaveformGenerator.cpp
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
    src/ai/LlamaCppProvider.cpp
    src/ai/AdvancedAI.cpp
    src/plugins/PluginManager.cpp
    src/render/graph/RenderGraph.cpp
    src/render/RenderManager.cpp
    src/render/ExportSettings.cpp
    src/render/ExportPresets.cpp
    src/render/FfmpegExporter.cpp
    src/render/TimelineExporter.cpp
    src/render/TimelineCompositor.cpp
    src/render/TextComposer.cpp
    src/render/HardwareCapabilities.cpp
    src/render/RenderExecutor.cpp
)

if(CCOS_ENABLE_LOCAL_AUTOMATION_API)
    list(APPEND CCOS_SOURCES src/api/LocalAutomationApi.cpp)
endif()

if(NOT CCOS_ENABLE_CULTURAL_MEDIA_API)
    list(REMOVE_ITEM CCOS_SOURCES src/api/CulturalMediaApi.cpp)
endif()

if(CCOS_ENABLE_VISION)
    find_package(OpenCV 4.8 REQUIRED COMPONENTS core imgproc video dnn imgcodecs)
    find_package(MediaPipe REQUIRED)
    if(NOT TARGET MediaPipe::mediapipe)
        message(FATAL_ERROR "CCOS_ENABLE_VISION requires the MediaPipe::mediapipe CMake target")
    endif()
    list(APPEND CCOS_SOURCES src/vision/ComputerVisionApi.cpp)
endif()

if(CCOS_ENABLE_EXPERIMENTAL_PREVIEW)
    list(APPEND CCOS_SOURCES src/preview/cache/FrameCache.cpp src/preview/pipeline/PreviewPipeline.cpp)
endif()
if(CCOS_ENABLE_RENDER_SCHEDULER)
    list(APPEND CCOS_SOURCES src/render/scheduler/RenderScheduler.cpp)
endif()

add_library(ccos_core ${CCOS_SOURCES})
target_include_directories(ccos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(CCOS_ENABLE_VISION)
    target_include_directories(ccos_core PUBLIC ${OpenCV_INCLUDE_DIRS})
endif()
target_compile_features(ccos_core PUBLIC cxx_std_20)
target_link_libraries(ccos_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Concurrent)
if(CCOS_ENABLE_MIT_FOUNDATION)
    target_link_libraries(ccos_core PUBLIC ccos_mit_foundation)
endif()
if(TARGET ccos_extended_open_source)
    target_link_libraries(ccos_core PUBLIC ccos_extended_open_source)
endif()
if(CCOS_ENABLE_MIT_MEDIA_3D AND TARGET ccos_mit_media_3d)
    target_link_libraries(ccos_core PUBLIC ccos_mit_media_3d)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_MIT_MEDIA_3D)
endif()
if(CCOS_ENABLE_MIT_TOOLING AND TARGET ccos_mit_tooling)
    target_link_libraries(ccos_core PUBLIC ccos_mit_tooling)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_MIT_TOOLING)
endif()
if(CCOS_ENABLE_MINIAUDIO AND TARGET ccos_miniaudio)
    target_link_libraries(ccos_core PUBLIC ccos_miniaudio)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_MINIAUDIO)
endif()
if(CCOS_ENABLE_ONNXRUNTIME AND TARGET ccos_onnxruntime)
    target_link_libraries(ccos_core PUBLIC ccos_onnxruntime)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_ONNXRUNTIME)
endif()
if(CCOS_ENABLE_LOCAL_AUTOMATION_API)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_LOCAL_AUTOMATION_API)
endif()
if(CCOS_ENABLE_CULTURAL_MEDIA_API)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_CULTURAL_MEDIA_API)
endif()
if(CCOS_ENABLE_LLAMA_SERVER_PROFILE)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_LLAMA_SERVER_PROFILE)
endif()
if(CCOS_ENABLE_VISION)
    target_link_libraries(ccos_core PUBLIC ${OpenCV_LIBS} MediaPipe::mediapipe)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_VISION)
endif()
if(CCOS_ENABLE_EXPERIMENTAL_PREVIEW)
    target_compile_definitions(ccos_core PUBLIC CCOS_HAS_EXPERIMENTAL_PREVIEW)
endif()
if(TARGET ccos_professional_interchange)
    target_link_libraries(ccos_core PUBLIC ccos_professional_interchange)
endif()

if(MSVC)
    target_compile_options(ccos_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(ccos_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(ccos_editor src/app/main.cpp src/ui/MainWindow.cpp src/ui/MainWindow.hpp)
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
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    find_package(GTest 1.14 QUIET)
    if(NOT GTest_FOUND AND CCOS_FETCH_GTEST)
        include(FetchContent)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
        FetchContent_MakeAvailable(googletest)
    endif()
    if(NOT TARGET GTest::gtest_main)
        message(FATAL_ERROR "GoogleTest is required. Install GTest or configure with -DCCOS_FETCH_GTEST=ON.")
    endif()
    add_executable(ccos_tests
        tests/core/TimeTests.cpp
        tests/core/CommandStackTests.cpp
        tests/project/ProjectTests.cpp
        tests/project/ProjectIdentityPersistenceTests.cpp
        tests/project/ProjectClipPropertiesTests.cpp
        tests/project/ProjectRecoveryTests.cpp
        tests/unit/project/ProjectSchemaTest.cpp
        tests/timeline/TimelineTests.cpp
        tests/timeline/TimelineEditorTests.cpp
        tests/timeline/KeyframeTests.cpp
        tests/media/MediaCacheTests.cpp
        tests/media/MediaImporterTests.cpp
        tests/media/MediaProbeTests.cpp
        tests/media/ThumbnailGeneratorTests.cpp
        tests/media/WaveformGeneratorTests.cpp
        tests/effects/BuiltinEffectsTests.cpp
        tests/text/SubtitleParserTests.cpp
        tests/unit/core/JobSystemSmokeTest.cpp
        tests/unit/core/ProcessRunnerTest.cpp
        tests/unit/core/PipelineGraphTest.cpp
        tests/unit/core/AIToolingSafetyTest.cpp
        tests/unit/render/RenderGraphSafetyTest.cpp
        tests/unit/render/ExportSettingsTest.cpp
        tests/unit/render/ExportPresetsTest.cpp
        tests/unit/api/EditorApiTest.cpp
        tests/unit/api/CreativeMediaApiTest.cpp
        tests/unit/core/MitFoundationSmokeTest.cpp
        tests/unit/api/NasaMediaApiTest.cpp
        tests/unit/api/CulturalMediaApiTest.cpp
    )
    if(CCOS_ENABLE_LOCAL_AUTOMATION_API)
        target_sources(ccos_tests PRIVATE tests/unit/api/LocalAutomationApiTest.cpp)
    endif()
    target_link_libraries(ccos_tests PRIVATE ccos_core GTest::gtest_main Qt6::Test)
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
