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

# Find required packages including new MIT libraries
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Multimedia MultimediaWidgets Svg Concurrent)
find_package(OpenCV 4.8 REQUIRED COMPONENTS core imgproc video dnn imgcodecs)
find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

# Optional packages for enhanced features
find_package(MediaPipe QUIET)
find_package(Skia QUIET)
find_package(RLottie QUIET)
find_package(NanoSVG QUIET)

add_library(ccos_core
    # API Layer
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
    
    # Computer Vision API (NEW - MIT Libraries)
    src/vision/ComputerVisionApi.cpp
    
    # Professional Audio API (NEW - MIT Libraries)
    src/audio_pro/AudioProcessingApi.cpp
    
    # Graphics & Animation API (NEW - MIT Libraries)
    src/graphics/GraphicsApi.cpp
    
    # Core
    src/core/Time.cpp
    src/core/Uuid.cpp
    src/core/CommandStack.cpp
    
    # Project
    src/project/Project.cpp
    src/project/ProjectSerializer.cpp
    
    # Media
    src/media/MediaAsset.cpp
    src/media/MediaImporter.cpp
    src/media/MediaProbe.cpp
    src/media/MediaCache.cpp
    src/media/ProxyManager.cpp
    src/media/MediaDerivatives.cpp
    
    # Timeline
    src/timeline/Clip.cpp
    src/timeline/Track.cpp
    src/timeline/Timeline.cpp
    src/timeline/TimelineEditor.cpp
    src/timeline/Keyframe.cpp
    
    # Audio
    src/audio/AudioMixer.cpp
    
    # Effects
    src/effects/BuiltinEffects.cpp
    
    # Transitions
    src/transitions/Transition.cpp
    
    # Text
    src/text/SubtitleParser.cpp
    
    # AI/ML
    src/ai/WhisperAdapter.cpp
    src/ai/OpenAICompatibleProvider.cpp
    src/ai/AdvancedAI.cpp
    
    # Plugins
    src/plugins/PluginManager.cpp
    
    # Render
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
    ${OpenCV_LIBS}
    ZLIB::ZLIB
)

# Optional ML/Vision libraries
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
    target_compile_options(ccos_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Editor executable
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
    target_compile_options(ccos_editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# CLI executable
if(CCOS_BUILD_CLI)
    add_executable(ccos_cli src/cli/main.cpp)
    target_link_libraries(ccos_cli PRIVATE ccos_core Qt6::Core)
    target_include_directories(ccos_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    install(TARGETS ccos_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

include(GNUInstallDirs)
install(TARGETS ccos_editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)
