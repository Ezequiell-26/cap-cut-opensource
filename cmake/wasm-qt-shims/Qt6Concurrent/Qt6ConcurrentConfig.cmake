# CCOS WebAssembly compatibility shim.
# Qt 6.8's WebAssembly package does not ship Qt Concurrent. The CCOS web
# source set also excludes the native JobSystem/PipelineGraph/ProcessRunner
# implementations that depend on it, so the editor does not need the module
# at link time. Keep the target available because the shared CMakeLists keeps
# the desktop component list intact.
if(NOT TARGET Qt6::Concurrent)
    add_library(Qt6::Concurrent INTERFACE IMPORTED)
endif()

set(Qt6Concurrent_FOUND TRUE)
set(Qt6Concurrent_VERSION "6.8.3")
set(Qt6Concurrent_VERSION_MAJOR 6)
set(Qt6Concurrent_VERSION_MINOR 8)
set(Qt6Concurrent_VERSION_PATCH 3)
set(Qt6Concurrent_VERSION_STRING "6.8.3")
set(Qt6Concurrent_LIBRARIES "Qt6::Concurrent")
