# WebAssembly-only integration for the existing C++/Qt editor.
# The native desktop UI remains untouched. CMake defers source selection until
# the main project has created the ccos_editor target.

function(ccos_enable_webassembly_ui)
    if(NOT EMSCRIPTEN OR NOT TARGET ccos_editor)
        return()
    endif()

    set(CCOS_DESKTOP_UI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/MainWindow.cpp")
    set(CCOS_WEB_UI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/WebMainWindow.cpp")

    if(EXISTS "${CCOS_DESKTOP_UI_SOURCE}")
        # Keep the desktop implementation in the target source list for IDE
        # visibility, but never compile it in the WebAssembly target.
        set_property(SOURCE "${CCOS_DESKTOP_UI_SOURCE}" PROPERTY HEADER_FILE_ONLY TRUE)
    endif()

    if(NOT EXISTS "${CCOS_WEB_UI_SOURCE}")
        message(FATAL_ERROR "CCOS WebAssembly UI source is missing: ${CCOS_WEB_UI_SOURCE}")
    endif()

    target_sources(ccos_editor PRIVATE "${CCOS_WEB_UI_SOURCE}")
    target_compile_definitions(ccos_editor PRIVATE CCOS_PLATFORM_WEB=1)
endfunction()

cmake_language(DEFER CALL ccos_enable_webassembly_ui)
