include(FetchContent)

# Optional extensions. They are deliberately OFF by default so a normal CCOS
# checkout keeps the existing build footprint and network requirements.
option(CCOS_ENABLE_MIT_UI_EXTENSIONS "Enable Dear ImGui + ImGuizmo + ImPlot integration" OFF)
option(CCOS_ENABLE_MIT_DIAGNOSTICS "Enable cpptrace crash/stack diagnostics" OFF)
option(CCOS_ENABLE_MIT_STORAGE "Enable unordered_dense/date/foonathan memory helpers" OFF)
option(CCOS_ENABLE_MIT_COMPRESSION "Enable optional libdeflate compression backend" OFF)
option(CCOS_ENABLE_PRO_TEXT "Use locally audited FreeType + HarfBuzz + libass" OFF)
option(CCOS_ENABLE_PRO_IMAGE_IO "Use locally audited OpenEXR + OpenImageIO + AVIF/WebP/JXL" OFF)
option(CCOS_ENABLE_PRO_AUDIO_IO "Use locally audited RtAudio + RtMidi + libsamplerate + SpeexDSP + RNNoise + KissFFT" OFF)
option(CCOS_ENABLE_PRO_CODECS "Use locally audited dav1d + SVT-AV1" OFF)
option(CCOS_ENABLE_PRO_STORAGE "Use locally audited zstd + lz4 + xxHash + libarchive + libzip" OFF)

add_library(ccos_extended_open_source INTERFACE)

if(CCOS_ENABLE_MIT_UI_EXTENSIONS)
    set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(IMGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    if(NOT TARGET ccos_imgui)
        add_library(ccos_imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        )
        target_include_directories(ccos_imgui PUBLIC ${imgui_SOURCE_DIR})
        target_compile_features(ccos_imgui PUBLIC cxx_std_20)
    endif()

    FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG 1.83
        GIT_SHALLOW TRUE
    )
    FetchContent_Declare(
        implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
        GIT_TAG v1.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imguizmo implot)

    if(NOT TARGET ccos_imguizmo)
        add_library(ccos_imguizmo STATIC
            ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp
        )
        target_include_directories(ccos_imguizmo PUBLIC ${imguizmo_SOURCE_DIR}/src)
        target_link_libraries(ccos_imguizmo PUBLIC ccos_imgui)
        target_compile_features(ccos_imguizmo PUBLIC cxx_std_20)
    endif()

    if(NOT TARGET ccos_implot)
        add_library(ccos_implot STATIC
            ${implot_SOURCE_DIR}/implot.cpp
            ${implot_SOURCE_DIR}/implot_items.cpp
        )
        target_include_directories(ccos_implot PUBLIC ${implot_SOURCE_DIR})
        target_link_libraries(ccos_implot PUBLIC ccos_imgui)
        target_compile_features(ccos_implot PUBLIC cxx_std_20)
    endif()

    target_link_libraries(ccos_extended_open_source INTERFACE ccos_imgui ccos_imguizmo ccos_implot)
    target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_IMGUI CCOS_HAS_IMGUIZMO CCOS_HAS_IMPLOT)
endif()

if(CCOS_ENABLE_MIT_DIAGNOSTICS)
    FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
        GIT_TAG v1.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(cpptrace)
    if(TARGET cpptrace::cpptrace)
        target_link_libraries(ccos_extended_open_source INTERFACE cpptrace::cpptrace)
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_CPPTRACE)
    endif()
endif()

if(CCOS_ENABLE_MIT_STORAGE)
    FetchContent_Declare(
        unordered_dense
        GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
        GIT_TAG v4.9.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(unordered_dense)
    if(NOT TARGET ccos_unordered_dense)
        add_library(ccos_unordered_dense INTERFACE)
        target_include_directories(ccos_unordered_dense INTERFACE ${unordered_dense_SOURCE_DIR}/include)
    endif()

    FetchContent_Declare(
        date
        GIT_REPOSITORY https://github.com/HowardHinnant/date.git
        GIT_TAG v3.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(date)

    FetchContent_Declare(
        foonathan_memory
        GIT_REPOSITORY https://github.com/foonathan/memory.git
        GIT_TAG v0.7-4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(foonathan_memory)

    target_link_libraries(ccos_extended_open_source INTERFACE ccos_unordered_dense)
    target_include_directories(ccos_extended_open_source INTERFACE
        ${date_SOURCE_DIR}/include
        ${foonathan_memory_SOURCE_DIR}/include
    )
    target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_UNORDERED_DENSE CCOS_HAS_DATE CCOS_HAS_FOONATHAN_MEMORY)
endif()

if(CCOS_ENABLE_MIT_COMPRESSION)
    FetchContent_Declare(
        libdeflate
        GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
        GIT_TAG v1.24
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(libdeflate)
    if(TARGET deflate OR TARGET libdeflate_shared OR TARGET libdeflate_static)
        if(TARGET libdeflate_static)
            target_link_libraries(ccos_extended_open_source INTERFACE libdeflate_static)
        elseif(TARGET libdeflate_shared)
            target_link_libraries(ccos_extended_open_source INTERFACE libdeflate_shared)
        elseif(TARGET deflate)
            target_link_libraries(ccos_extended_open_source INTERFACE deflate)
        endif()
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBDEFLATE)
    endif()
endif()

# Professional libraries are discovered from the host toolchain. We do not
# implicitly fetch codec/image/audio stacks because their transitive licenses,
# patent terms and platform backends need release-time audit.
if(CCOS_ENABLE_PRO_TEXT)
    find_package(Freetype QUIET)
    find_package(harfbuzz CONFIG QUIET)
    if(TARGET Freetype::Freetype)
        target_link_libraries(ccos_extended_open_source INTERFACE Freetype::Freetype)
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_FREETYPE)
    endif()
    if(TARGET harfbuzz::harfbuzz)
        target_link_libraries(ccos_extended_open_source INTERFACE harfbuzz::harfbuzz)
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_HARFBUZZ)
    endif()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBASS QUIET libass)
        if(LIBASS_FOUND)
            target_include_directories(ccos_extended_open_source INTERFACE ${LIBASS_INCLUDE_DIRS})
            target_link_directories(ccos_extended_open_source INTERFACE ${LIBASS_LIBRARY_DIRS})
            target_link_libraries(ccos_extended_open_source INTERFACE ${LIBASS_LIBRARIES})
            target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBASS)
        endif()
    endif()
endif()

if(CCOS_ENABLE_PRO_IMAGE_IO)
    find_package(OpenEXR CONFIG QUIET)
    if(TARGET OpenEXR::OpenEXR)
        target_link_libraries(ccos_extended_open_source INTERFACE OpenEXR::OpenEXR)
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_OPENEXR)
    endif()

    find_package(OpenImageIO CONFIG QUIET)
    if(TARGET OpenImageIO::OpenImageIO)
        target_link_libraries(ccos_extended_open_source INTERFACE OpenImageIO::OpenImageIO)
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_OPENIMAGEIO)
    endif()

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBAVIF QUIET libavif)
        if(LIBAVIF_FOUND)
            target_include_directories(ccos_extended_open_source INTERFACE ${LIBAVIF_INCLUDE_DIRS})
            target_link_directories(ccos_extended_open_source INTERFACE ${LIBAVIF_LIBRARY_DIRS})
            target_link_libraries(ccos_extended_open_source INTERFACE ${LIBAVIF_LIBRARIES})
            target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBAVIF)
        endif()
        pkg_check_modules(LIBWEBP QUIET libwebp)
        if(LIBWEBP_FOUND)
            target_include_directories(ccos_extended_open_source INTERFACE ${LIBWEBP_INCLUDE_DIRS})
            target_link_directories(ccos_extended_open_source INTERFACE ${LIBWEBP_LIBRARY_DIRS})
            target_link_libraries(ccos_extended_open_source INTERFACE ${LIBWEBP_LIBRARIES})
            target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBWEBP)
        endif()
        pkg_check_modules(LIBJXL QUIET libjxl)
        if(LIBJXL_FOUND)
            target_include_directories(ccos_extended_open_source INTERFACE ${LIBJXL_INCLUDE_DIRS})
            target_link_directories(ccos_extended_open_source INTERFACE ${LIBJXL_LIBRARY_DIRS})
            target_link_libraries(ccos_extended_open_source INTERFACE ${LIBJXL_LIBRARIES})
            target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBJXL)
        endif()
    endif()
endif()

if(CCOS_ENABLE_PRO_AUDIO_IO OR CCOS_ENABLE_PRO_CODECS OR CCOS_ENABLE_PRO_STORAGE)
    find_package(PkgConfig QUIET)
endif()

if(CCOS_ENABLE_PRO_AUDIO_IO AND PkgConfig_FOUND)
    pkg_check_modules(RTAUDIO QUIET rtaudio)
    if(RTAUDIO_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${RTAUDIO_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${RTAUDIO_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${RTAUDIO_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_RTAUDIO)
    endif()
    pkg_check_modules(RTMIDI QUIET rtmidi)
    if(RTMIDI_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${RTMIDI_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${RTMIDI_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${RTMIDI_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_RTMIDI)
    endif()
    pkg_check_modules(LIBSAMPLERATE QUIET samplerate)
    if(LIBSAMPLERATE_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${LIBSAMPLERATE_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${LIBSAMPLERATE_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${LIBSAMPLERATE_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBSAMPLERATE)
    endif()
    pkg_check_modules(SPEEXDSP QUIET speexdsp)
    if(SPEEXDSP_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${SPEEXDSP_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${SPEEXDSP_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${SPEEXDSP_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_SPEEXDSP)
    endif()
    pkg_check_modules(RNNOISE QUIET rnnoise)
    if(RNNOISE_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${RNNOISE_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${RNNOISE_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${RNNOISE_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_RNNOISE)
    endif()
    pkg_check_modules(KISSFFT QUIET kissfft)
    if(KISSFFT_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${KISSFFT_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${KISSFFT_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${KISSFFT_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_KISSFFT)
    endif()
endif()

if(CCOS_ENABLE_PRO_CODECS AND PkgConfig_FOUND)
    pkg_check_modules(DAV1D QUIET dav1d)
    if(DAV1D_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${DAV1D_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${DAV1D_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${DAV1D_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_DAV1D)
    endif()
    pkg_check_modules(SVTAV1 QUIET SvtAv1Enc)
    if(SVTAV1_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${SVTAV1_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${SVTAV1_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${SVTAV1_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_SVT_AV1)
    endif()
endif()

if(CCOS_ENABLE_PRO_STORAGE AND PkgConfig_FOUND)
    pkg_check_modules(ZSTD QUIET libzstd)
    if(ZSTD_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${ZSTD_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${ZSTD_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${ZSTD_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_ZSTD)
    endif()
    pkg_check_modules(LZ4 QUIET liblz4)
    if(LZ4_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${LZ4_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${LZ4_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${LZ4_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LZ4)
    endif()
    pkg_check_modules(XXHASH QUIET libxxhash)
    if(XXHASH_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${XXHASH_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${XXHASH_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${XXHASH_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_XXHASH)
    endif()
    pkg_check_modules(LIBARCHIVE QUIET libarchive)
    if(LIBARCHIVE_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${LIBARCHIVE_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${LIBARCHIVE_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${LIBARCHIVE_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBARCHIVE)
    endif()
    pkg_check_modules(LIBZIP QUIET libzip)
    if(LIBZIP_FOUND)
        target_include_directories(ccos_extended_open_source INTERFACE ${LIBZIP_INCLUDE_DIRS})
        target_link_directories(ccos_extended_open_source INTERFACE ${LIBZIP_LIBRARY_DIRS})
        target_link_libraries(ccos_extended_open_source INTERFACE ${LIBZIP_LIBRARIES})
        target_compile_definitions(ccos_extended_open_source INTERFACE CCOS_HAS_LIBZIP)
    endif()
endif()
