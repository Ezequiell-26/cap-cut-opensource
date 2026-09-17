# Optional HarfBuzz text-shaping bridge. Kept separate from the editor UI so
# the text domain can expose glyph runs without depending on widget code.

function(ccos_configure_text_shaping target_name)
    if(NOT CCOS_ENABLE_PRO_TEXT)
        return()
    endif()

    find_package(PkgConfig QUIET)
    if(NOT PkgConfig_FOUND)
        message(STATUS "CCOS text shaping: pkg-config unavailable; HarfBuzz backend disabled")
        return()
    endif()

    pkg_check_modules(CCOS_HARFBUZZ QUIET harfbuzz)
    if(NOT CCOS_HARFBUZZ_FOUND)
        message(STATUS "CCOS text shaping: HarfBuzz not found; Unicode shaping fallback remains active")
        return()
    endif()

    target_include_directories(${target_name} PRIVATE ${CCOS_HARFBUZZ_INCLUDE_DIRS})
    target_link_directories(${target_name} PRIVATE ${CCOS_HARFBUZZ_LIBRARY_DIRS})
    target_link_libraries(${target_name} PRIVATE ${CCOS_HARFBUZZ_LIBRARIES})
    target_compile_definitions(${target_name} PRIVATE CCOS_HAS_HARFBUZZ)
endfunction()
