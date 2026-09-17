include(FetchContent)

option(CCOS_ENABLE_MIT_FOUNDATION "Fetch the curated MIT-licensed C++ foundation stack" ON)
option(CCOS_ENABLE_MIT_MEDIA_3D "Fetch optional MIT C++ libraries for 3D/motion-graphics asset processing" OFF)
option(CCOS_ENABLE_MIT_TOOLING "Fetch optional MIT C++ tooling libraries for CLI/XML/test infrastructure" OFF)
option(CCOS_ENABLE_MINIAUDIO "Make miniaudio available to optional audio device backends" OFF)
option(CCOS_ENABLE_ONNXRUNTIME "Enable the optional ONNX Runtime integration" OFF)

if(NOT CCOS_ENABLE_MIT_FOUNDATION)
    return()
endif()

message(STATUS "CCOS: enabling curated permissive C++ foundation dependencies")

# Versions are pinned intentionally. Review the upstream license file and
# transitive dependencies before upgrading any entry.
# NOTE: this group is permissive, not "MIT-only": cpp-httplib is BSD-3-Clause.
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 GIT_SHALLOW TRUE)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 12.2.0 GIT_SHALLOW TRUE)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.17.0 GIT_SHALLOW TRUE)
FetchContent_Declare(taskflow GIT_REPOSITORY https://github.com/taskflow/taskflow.git GIT_TAG v4.1.0 GIT_SHALLOW TRUE)
FetchContent_Declare(magic_enum GIT_REPOSITORY https://github.com/Neargye/magic_enum.git GIT_TAG v0.9.8 GIT_SHALLOW TRUE)
FetchContent_Declare(cpp_httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git GIT_TAG v0.56.0 GIT_SHALLOW TRUE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.3 GIT_SHALLOW TRUE)
FetchContent_Declare(tomlplusplus GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git GIT_TAG v3.4.0 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json fmt spdlog taskflow magic_enum cpp_httplib glm tomlplusplus)

if(NOT TARGET ccos_mit_foundation)
    add_library(ccos_mit_foundation INTERFACE)
    target_link_libraries(ccos_mit_foundation INTERFACE nlohmann_json::nlohmann_json fmt::fmt spdlog::spdlog glm::glm tomlplusplus::tomlplusplus)
    target_include_directories(ccos_mit_foundation INTERFACE
        ${magic_enum_SOURCE_DIR}/include
        ${taskflow_SOURCE_DIR}
        ${cpp_httplib_SOURCE_DIR}
    )
endif()

if(CCOS_ENABLE_MIT_MEDIA_3D)
    FetchContent_Declare(entt GIT_REPOSITORY https://github.com/skypjack/entt.git GIT_TAG v4.0.0 GIT_SHALLOW TRUE)
    FetchContent_Declare(meshoptimizer GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git GIT_TAG v1.2 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(entt meshoptimizer)
    if(NOT TARGET ccos_mit_media_3d)
        add_library(ccos_mit_media_3d INTERFACE)
        target_include_directories(ccos_mit_media_3d INTERFACE ${entt_SOURCE_DIR}/src ${meshoptimizer_SOURCE_DIR}/src)
    endif()
endif()

if(CCOS_ENABLE_MIT_TOOLING)
    # MIT: lightweight command-line parsing, XML, and test infrastructure.
    FetchContent_Declare(cxxopts GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git GIT_TAG v3.3.1 GIT_SHALLOW TRUE)
    FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.4.12 GIT_SHALLOW TRUE)
    FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml.git GIT_TAG v1.15 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(cxxopts doctest pugixml)
    if(NOT TARGET ccos_mit_tooling)
        add_library(ccos_mit_tooling INTERFACE)
        target_link_libraries(ccos_mit_tooling INTERFACE cxxopts pugixml)
        target_include_directories(ccos_mit_tooling INTERFACE ${doctest_SOURCE_DIR})
    endif()
endif()

if(CCOS_ENABLE_MINIAUDIO)
    FetchContent_Declare(miniaudio GIT_REPOSITORY https://github.com/mackron/miniaudio.git GIT_TAG 0.11.25 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(miniaudio)
    if(NOT TARGET ccos_miniaudio)
        add_library(ccos_miniaudio INTERFACE)
        target_include_directories(ccos_miniaudio INTERFACE ${miniaudio_SOURCE_DIR})
    endif()
endif()

if(CCOS_ENABLE_ONNXRUNTIME)
    # ONNX Runtime is MIT at the top-level repository, but a distribution may
    # contain additional execution-provider dependencies and licenses. Never
    # download it implicitly; use an audited local installation.
    find_package(onnxruntime CONFIG REQUIRED)
    if(TARGET onnxruntime::onnxruntime AND NOT TARGET ccos_onnxruntime)
        add_library(ccos_onnxruntime INTERFACE)
        target_link_libraries(ccos_onnxruntime INTERFACE onnxruntime::onnxruntime)
    endif()
endif()
