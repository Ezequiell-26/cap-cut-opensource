# CMake compatibility shim for Qt's prebuilt Emscripten package.
# Qt's own Qt 6.8 FindEGL implementation treats EGL as an implicit system
# interface on Emscripten; avoid running a native-style try_compile probe.
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "CCOS WASM FindEGL shim loaded outside Emscripten")
endif()

set(EGL_FOUND TRUE)
set(EGL_VERSION "1.5")
set(EGL_INCLUDE_DIRS "${CMAKE_SYSTEM_INCLUDE_PATH}")
set(EGL_LIBRARIES "")
set(EGL_DEFINITIONS "")

if(NOT DEFINED EGL_INCLUDE_DIR OR EGL_INCLUDE_DIR STREQUAL "")
    if(DEFINED ENV{EMSDK})
        set(EGL_INCLUDE_DIR "$ENV{EMSDK}/upstream/emscripten/cache/sysroot/include" CACHE PATH "Emscripten EGL include directory")
    endif()
endif()
set(EGL_INCLUDE_DIRS "${EGL_INCLUDE_DIR}")

if(NOT TARGET EGL::EGL)
    add_library(EGL::EGL INTERFACE IMPORTED)
    if(EGL_INCLUDE_DIR)
        set_property(TARGET EGL::EGL PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}")
    endif()
endif()

set(EGL_FOUND TRUE CACHE BOOL "EGL is provided implicitly by Emscripten" FORCE)
set(EGL_VERSION_STRING "${EGL_VERSION}")
