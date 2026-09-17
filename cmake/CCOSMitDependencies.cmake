include(FetchContent)

option(CCOS_ENABLE_MIT_FOUNDATION "Fetch the curated MIT-licensed C++ foundation stack" ON)
option(CCOS_ENABLE_MINIAUDIO "Make miniaudio available to optional audio device backends" OFF)
option(CCOS_ENABLE_ONNXRUNTIME "Enable the optional ONNX Runtime integration" OFF)

if(NOT CCOS_ENABLE_MIT_FOUNDATION)
    return()
endif()

message(STATUS "CCOS: enabling curated MIT foundation dependencies")

# Versions are pinned intentionally. Review license files before upgrading.
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.2.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    taskflow
    GIT_REPOSITORY https://github.com/taskflow/taskflow.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.8
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.56.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json fmt spdlog taskflow magic_enum cpp_httplib)

if(NOT TARGET ccos_mit_foundation)
    add_library(ccos_mit_foundation INTERFACE)
    target_link_libraries(ccos_mit_foundation INTERFACE
        nlohmann_json::nlohmann_json
        fmt::fmt
        spdlog::spdlog
    )
    target_include_directories(ccos_mit_foundation INTERFACE
        ${magic_enum_SOURCE_DIR}/include
        ${taskflow_SOURCE_DIR}
    )
endif()

if(CCOS_ENABLE_MINIAUDIO)
    FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(miniaudio)
    add_library(ccos_miniaudio INTERFACE)
    target_include_directories(ccos_miniaudio INTERFACE ${miniaudio_SOURCE_DIR})
endif()

if(CCOS_ENABLE_ONNXRUNTIME)
    # ONNX Runtime is top-level MIT, but its distributions can contain
    # additional third-party/execution-provider licenses. Do not download it
    # implicitly. Point CMAKE_PREFIX_PATH to an audited installation.
    find_package(onnxruntime CONFIG REQUIRED)
    if(TARGET onnxruntime::onnxruntime)
        add_library(ccos_onnxruntime INTERFACE)
        target_link_libraries(ccos_onnxruntime INTERFACE onnxruntime::onnxruntime)
    endif()
endif()
