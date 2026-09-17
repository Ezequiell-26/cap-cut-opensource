# Third-party license inventory

CCOS source code is MIT. Dependencies remain under their own licenses. This inventory covers the dependency families referenced by the current CMake configuration and the optional extension matrix; the exact pinned version and transitive dependencies must still be audited before release.

## Core dependencies

| Dependency | Purpose | License |
|---|---|---|
| Qt 6 | Desktop UI, networking, multimedia | LGPL/GPL/commercial; distribution obligations depend on modules and configuration |
| FFmpeg | Demux/decode/encode/filtering | LGPL/GPL depending on enabled components; keep GPL components disabled unless intentionally distributing under compatible terms |
| GoogleTest | Tests | BSD-3-Clause |
| nlohmann/json | JSON project/API data | MIT |
| fmt | Formatting | MIT |
| spdlog | Logging | MIT |
| Taskflow | Parallel task graph execution | MIT |
| magic_enum | Compile-time enum reflection | MIT |
| cpp-httplib | Local HTTP automation | MIT |
| GLM | Math/graphics primitives | MIT |
| toml++ | Configuration | MIT |

## Optional MIT / permissive extensions

| Dependency | Purpose | License | CMake option |
|---|---|---|---|
| EnTT | ECS / runtime data-oriented 3D support | MIT | `CCOS_ENABLE_MIT_MEDIA_3D` |
| meshoptimizer | Mesh/geometry optimization | MIT | `CCOS_ENABLE_MIT_MEDIA_3D` |
| cxxopts | CLI argument parsing | MIT | `CCOS_ENABLE_MIT_TOOLING` |
| doctest | Lightweight supplementary tests | MIT | `CCOS_ENABLE_MIT_TOOLING` |
| pugixml | XML parsing for interchange/import tooling | MIT | `CCOS_ENABLE_MIT_TOOLING` |
| miniaudio | Optional audio device backend | Public Domain / MIT-0 | `CCOS_ENABLE_MINIAUDIO` |
| ONNX Runtime | Local ML inference runtime | MIT at the repository level; execution providers may introduce additional licenses | `CCOS_ENABLE_ONNXRUNTIME` |
| Dear ImGui | Technical/debug/content-creation UI | MIT | `CCOS_ENABLE_MIT_UI_EXTENSIONS` |
| ImGuizmo | Visual transform/gizmo/sequencer widgets | MIT | `CCOS_ENABLE_MIT_UI_EXTENSIONS` |
| cpptrace | Stack traces/crash diagnostics | MIT; backend dependencies can add obligations | `CCOS_ENABLE_MIT_DIAGNOSTICS` |
| martinus/unordered_dense | High-performance hash containers | MIT | `CCOS_ENABLE_MIT_STORAGE` |
| Tessil/robin-map | Robin Hood hash map/set | MIT | `CCOS_ENABLE_MIT_UTILITY_EXTENSIONS` |
| bombela/backward-cpp | Stack trace pretty-printer | MIT; optional unwind/debug libraries have their own licenses | `CCOS_ENABLE_MIT_UTILITY_EXTENSIONS` |
| HowardHinnant/date | Date/time utilities | MIT | `CCOS_ENABLE_MIT_STORAGE` |
| foonathan/memory | Allocator/memory utilities | MIT | `CCOS_ENABLE_MIT_STORAGE` |
| libdeflate | Fast DEFLATE/zlib/gzip compression | MIT | `CCOS_ENABLE_MIT_COMPRESSION` |
| sheredom/utf8.h | Single-header UTF-8 utilities | Public Domain / Unlicense | `CCOS_ENABLE_UTF8_HEADER` |
| HarfBuzz | Complex-script text shaping | MIT | `CCOS_ENABLE_PRO_TEXT` |

`robin-map` is currently at v1.4.1 upstream and is MIT. `backward-cpp` is MIT and can be used as a CMake dependency, but richer stack-resolution modes rely on platform debugging/unwind libraries. The CCOS default build therefore keeps both optional. citeturn514282search0turn514282search2turn107820search0

`sheredom/utf8.h` is public-domain/Unlicense rather than MIT. The CCOS CMake configuration pins the current audited upstream commit when the optional integration is enabled. citeturn346file0turn581819search0

## Optional professional integrations

| Dependency | Purpose | Current upstream version observed | License | CMake option |
|---|---|---|---|---|
| OpenTimelineIO | C++ timeline/interchange model and serialization | v0.18.1 | Modified Apache 2.0 | `CCOS_ENABLE_OPENTIMELINEIO` |
| OpenColorIO | Color management, LUT/config processing and GPU-capable transforms | v2.5.2 | BSD-3-Clause | `CCOS_ENABLE_OPENCOLORIO` |
| FreeType | Font rasterization and font services | audited local version required | FTL / GPL-2.0-or-later | `CCOS_ENABLE_PRO_TEXT` |
| libass | ASS/SSA subtitle rendering | v0.17.5 | ISC | `CCOS_ENABLE_PRO_TEXT` |
| OpenEXR | Professional HDR/VFX image format | v3.4.15 security release line | BSD-3-Clause | `CCOS_ENABLE_PRO_IMAGE_IO` |
| OpenImageIO | VFX-grade image I/O and processing | audited local version required | Apache-2.0 | `CCOS_ENABLE_PRO_IMAGE_IO` |
| libavif | AVIF encode/decode | v1.4.2 | BSD-style; inspect bundled codec dependencies | `CCOS_ENABLE_PRO_IMAGE_IO` |
| libwebp | WebP encode/decode | audited local version required | BSD-style + patent grant | `CCOS_ENABLE_PRO_IMAGE_IO` |
| libjxl | JPEG XL | audited local version required | BSD-3-Clause + patent terms | `CCOS_ENABLE_PRO_IMAGE_IO` |
| RtAudio | Real-time audio device I/O | audited local version required | permissive RtAudio license; review notice | `CCOS_ENABLE_PRO_AUDIO_IO` |
| RtMidi | MIDI input/output | audited local version required | permissive RtMidi license; review notice | `CCOS_ENABLE_PRO_AUDIO_IO` |
| libsamplerate | High-quality audio resampling | audited local version required | BSD-2-Clause | `CCOS_ENABLE_PRO_AUDIO_IO` |
| SpeexDSP | Audio DSP/noise/signal processing | audited local version required | BSD-3-Clause | `CCOS_ENABLE_PRO_AUDIO_IO` |
| RNNoise | Neural noise suppression | v0.2 | BSD-3-Clause | `CCOS_ENABLE_PRO_AUDIO_IO` |
| KissFFT | FFT/spectrum analysis | v131.1.0 | Revised BSD | `CCOS_ENABLE_PRO_AUDIO_IO` |
| dav1d | Fast AV1 decoder | audited local version required | BSD-2-Clause | `CCOS_ENABLE_PRO_CODECS` |
| SVT-AV1 | AV1 encoder | audited local version required | BSD-3-Clause Clear + AOM patent license | `CCOS_ENABLE_PRO_CODECS` |
| Zstandard | Project/cache compression | v1.5.7 | BSD | `CCOS_ENABLE_PRO_STORAGE` |
| LZ4 | Fast compression/decompression | audited local version required | BSD-2-Clause | `CCOS_ENABLE_PRO_STORAGE` |
| xxHash | Non-cryptographic hashing | audited local version required | BSD-2-Clause | `CCOS_ENABLE_PRO_STORAGE` |
| libarchive | Archive/container access | audited local version required | BSD-like / multi-license tree | `CCOS_ENABLE_PRO_STORAGE` |
| libzip | ZIP archive access | audited local version required | BSD-3-Clause | `CCOS_ENABLE_PRO_STORAGE` |

OpenTimelineIO is intentionally discovered from an audited local C++ installation instead of being fetched automatically. Its current CMake build exposes the `OTIO::opentimelineio` target. OpenColorIO is likewise discovered locally and is accepted only at version 2.5.2 or newer because 2.5.2 contains the upstream security fix for CVE-2026-42450.

The current extension CMake modules fetch only lightweight optional components and discover heavyweight codec/image/audio stacks from the host toolchain. This is intentional: blindly fetching large multimedia stacks would make release licensing, patent, and platform-backend auditing harder and would increase build fragility.

## Policy

1. Pin versions; do not consume floating branches in release builds.
2. Record the upstream repository, exact version/commit, SPDX identifier and license text before shipping.
3. Audit transitive dependencies, especially codec, GPU and execution-provider stacks.
4. Keep API/content licenses separate from software dependency licenses.
5. Preserve attribution and provider terms when importing media.
6. Do not claim an imported asset is MIT merely because its discovery API is free or open source.
7. For optional professional integrations, require a local audited installation and a minimum supported security version before release.
8. Run a release-time license scan against the final dependency graph.
