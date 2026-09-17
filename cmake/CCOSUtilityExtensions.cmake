include(FetchContent)

option(CCOS_ENABLE_MIT_UTILITY_EXTENSIONS "Enable robin-map and backward-cpp utility dependencies" OFF)
option(CCOS_ENABLE_UTF8_HEADER "Enable sheredom/utf8.h public-domain UTF-8 helpers" OFF)

add_library(ccos_utility_extensions INTERFACE)

if(CCOS_ENABLE_MIT_UTILITY_EXTENSIONS)
    FetchContent_Declare(
        robin_map
        GIT_REPOSITORY https://github.com/Tessil/robin-map.git
        GIT_TAG v1.4.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(robin_map)

    if(TARGET tsl::robin_map)
        target_link_libraries(ccos_utility_extensions INTERFACE tsl::robin_map)
    else()
        target_include_directories(ccos_utility_extensions INTERFACE ${robin_map_SOURCE_DIR}/include)
    endif()

    FetchContent_Declare(
        backward_cpp
        GIT_REPOSITORY https://github.com/bombela/backward-cpp.git
        GIT_TAG v1.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(backward_cpp)

    if(TARGET Backward::Interface)
        target_link_libraries(ccos_utility_extensions INTERFACE Backward::Interface)
    else()
        target_include_directories(ccos_utility_extensions INTERFACE ${backward_cpp_SOURCE_DIR})
    endif()
    target_compile_definitions(ccos_utility_extensions INTERFACE CCOS_HAS_ROBIN_MAP CCOS_HAS_BACKWARD_CPP)
endif()

if(CCOS_ENABLE_UTF8_HEADER)
    FetchContent_Declare(
        utf8_header
        GIT_REPOSITORY https://github.com/sheredom/utf8.h.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    # sheredom/utf8.h is a single public-domain header and does not publish a
    # stable release tag. Keep it opt-in and require release-time commit pinning
    # before this option is used in a reproducible release build.
    FetchContent_MakeAvailable(utf8_header)
    target_include_directories(ccos_utility_extensions INTERFACE ${utf8_header_SOURCE_DIR})
    target_compile_definitions(ccos_utility_extensions INTERFACE CCOS_HAS_UTF8_HEADER)
endif()
