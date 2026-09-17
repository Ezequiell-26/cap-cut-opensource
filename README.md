# CCOS

CCOS (C++ Creative Open Source) is a cross-platform open-source video editor built around a C++ media/timeline core and a Qt 6 desktop UI.

## Current status

The repository now has a real desktop application shell plus hardened media, project, render, API and automation foundations:

- C++20 + CMake + Qt 6
- `.ccos` JSON project persistence with bounded validation and atomic writes
- media-bin import model, media probing and relinking support
- video/audio timeline tracks, clip placement, rational timeline time and undo/redo
- crash recovery snapshots with project-scoped recovery files
- FFmpeg export for standalone assets and contiguous timelines
- proxy generation and bounded thumbnail generation with cache reuse
- runtime hardware encoder discovery for NVENC, QSV, AMF, VideoToolbox and VAAPI
- safe `videoCodec=auto` selection with software fallback
- headless `ccos-cli` inspection, validation, export and hardware/doctor operations
- optional MIT/permissive C++ foundation dependencies, kept behind CMake options where appropriate
- optional OpenTimelineIO and OpenColorIO integration boundaries
- API adapters for external media/utility services with bounded network policies
- GoogleTest coverage plus CI, security and hardening workflows

The architecture deliberately keeps decoding/rendering services, project state, media caching and automation boundaries independent from the Qt UI wherever practical. Advanced professional integrations remain optional rather than becoming mandatory contributor dependencies.

## Architecture

`src/core` contains reusable primitives. `src/media`, `src/timeline`, `src/project`, `src/render`, `src/audio`, `src/effects`, `src/text`, `src/plugins` and `src/ai` remain independent from the UI wherever practical.

## Build

Requirements:

- CMake 3.24+
- C++20 compiler
- Qt 6.5+
- GoogleTest 1.14+ when tests are enabled
- FFmpeg at runtime for probing, proxy creation and export

Configure with CMake and build the `ccos_editor` target. Enable `CCOS_BUILD_TESTS=ON` for the test suite.

The default configuration keeps heavyweight professional SDKs optional. See `CMakePresets.json`, `docs/HARDWARE_ACCELERATION.md`, `docs/PROFESSIONAL_INTERCHANGE.md` and `docs/licenses/THIRD_PARTY_LICENSES.md` for the supported integration boundaries.

## Design principles

- stable domain model before advanced features
- no blocking media work on the GUI thread for future long-running media services
- exact rational timeline time instead of floating-point frame positions
- optional services and AI; essential editing remains offline-capable
- explicit third-party licensing
- bounded subprocess/network work
- crash-safe project persistence and recovery
- runtime capability detection instead of hard-coded hardware assumptions
- tests for domain behavior and integration flows

## License

CCOS source code is MIT-licensed. See `LICENSE`. Third-party dependencies retain their own licenses and are listed in `docs/licenses/THIRD_PARTY_LICENSES.md`.
