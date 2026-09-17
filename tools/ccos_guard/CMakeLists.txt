cmake_minimum_required(VERSION 3.24)
project(CCOSGuard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ccos_guard main.cpp)
target_compile_features(ccos_guard PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ccos_guard PRIVATE /W4 /permissive-)
else()
    target_compile_options(ccos_guard PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

enable_testing()
add_test(NAME ccos_guard_repository
         COMMAND ccos_guard ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set_tests_properties(ccos_guard_repository PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
