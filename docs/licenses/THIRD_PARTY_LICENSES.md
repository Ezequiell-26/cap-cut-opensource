# Third-party license inventory

CCOS source code is MIT. Dependencies remain under their own licenses. This inventory covers the dependency families referenced by the current CMake configuration; the exact pinned version and transitive dependencies must still be audited before release.

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

## Optional professional integrations

| Dependency | Purpose | Current upstream version observed | License | CMake option |
|---|---|---|---|---|
| OpenTimelineIO | C++ timeline/interchange model and serialization | v0.18.1 | Modified Apache 2.0 | `CCOS_ENABLE_OPENTIMELINEIO` |
| OpenColorIO | Color management, LUT/config processing and GPU-capable transforms | v2.5.2 | BSD-3-Clause | `CCOS_ENABLE_OPENCOLORIO` |

OpenTimelineIO is intentionally discovered from an audited local C++ installation instead of being fetched automatically. Its current CMake build exposes the `OTIO::opentimelineio` target. OpenColorIO is likewise discovered locally and is accepted only at version 2.5.2 or newer because 2.5.2 contains the upstream security fix for CVE-2026-42450.

## Policy

1. Pin versions; do not consume floating branches in release builds.
2. Record the upstream repository, exact version/commit, SPDX identifier and license text before shipping.
3. Audit transitive dependencies, especially codec, GPU and execution-provider stacks.
4. Keep API/content licenses separate from software dependency licenses.
5. Preserve attribution and provider terms when importing media.
6. Do not claim an imported asset is MIT merely because its discovery API is free or open source.
7. For optional professional integrations, require a local audited installation and a minimum supported security version before release.
8. Run a release-time license scan against the final dependency graph.
