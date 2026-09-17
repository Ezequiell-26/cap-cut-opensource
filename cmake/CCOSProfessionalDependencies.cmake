# Optional professional interoperability/color-management dependencies.
# These are discovered from an audited local installation; they are never
# downloaded implicitly by the default build.

option(CCOS_ENABLE_OPENTIMELINEIO "Enable optional OpenTimelineIO C++ interchange support" OFF)
option(CCOS_ENABLE_OPENCOLORIO "Enable optional OpenColorIO color-management support" OFF)

if(CCOS_ENABLE_OPENTIMELINEIO)
    find_package(OpenTimelineIO CONFIG QUIET)
    if(NOT OpenTimelineIO_FOUND)
        message(FATAL_ERROR
            "CCOS_ENABLE_OPENTIMELINEIO=ON requires an installed OpenTimelineIO C++ package. "
            "Install an audited C++ build with OTIO_CXX_INSTALL=ON and OTIO_PYTHON_INSTALL=OFF.")
    endif()

    if(TARGET OTIO::opentimelineio)
        add_library(ccos_opentimelineio INTERFACE)
        target_link_libraries(ccos_opentimelineio INTERFACE OTIO::opentimelineio)
    else()
        message(FATAL_ERROR "OpenTimelineIO was found but OTIO::opentimelineio is unavailable")
    endif()
endif()

if(CCOS_ENABLE_OPENCOLORIO)
    find_package(OpenColorIO 2.5 CONFIG REQUIRED)
    if(NOT TARGET OpenColorIO::OpenColorIO)
        message(FATAL_ERROR "OpenColorIO was found but OpenColorIO::OpenColorIO is unavailable")
    endif()

    add_library(ccos_opencolorio INTERFACE)
    target_link_libraries(ccos_opencolorio INTERFACE OpenColorIO::OpenColorIO)

    if(OpenColorIO_VERSION VERSION_LESS "2.5.2")
        message(FATAL_ERROR
            "CCOS_ENABLE_OPENCOLORIO requires OpenColorIO >= 2.5.2; older 2.5.x builds are not accepted.")
    endif()
endif()

if(TARGET ccos_opentimelineio OR TARGET ccos_opencolorio)
    add_library(ccos_professional_interchange INTERFACE)
    if(TARGET ccos_opentimelineio)
        target_link_libraries(ccos_professional_interchange INTERFACE ccos_opentimelineio)
        target_compile_definitions(ccos_professional_interchange INTERFACE CCOS_HAS_OPENTIMELINEIO)
    endif()
    if(TARGET ccos_opencolorio)
        target_link_libraries(ccos_professional_interchange INTERFACE ccos_opencolorio)
        target_compile_definitions(ccos_professional_interchange INTERFACE CCOS_HAS_OPENCOLORIO)
    endif()
endif()
