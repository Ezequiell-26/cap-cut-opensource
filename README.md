# CCOS

CCOS (C++ Creative Open Source) is a cross-platform open-source video editor built around a C++ media/timeline core and a Qt 6 desktop UI.

## Current status

The repository now has a real desktop application shell plus hardened media, project, render, API and automation foundations:

- C++20 + CMake + Qt 6
- `.ccos` JSON project persistence with bounded validation and atomic writes
- media-bin import model, media probing and relinking support
- video/audio timeline tracks, clip placement, rational timeline time and undo/redo
- crash recovery snapshots with project-scoped recovery files
- dirty-state protection for save, close, New Project and Open Project flows
- FFmpeg export for standalone assets and contiguous timelines
- proxy generation, cached thumbnails and cached audio waveforms
- media-cache keys fingerprint local source size and modification time to invalidate stale derived media
- runtime hardware encoder discovery for NVENC, QSV, AMF, VideoToolbox and VAAPI
- safe `videoCodec=auto` selection with software fallback
- typed export presets for common horizontal, vertical and WebM workflows
- headless `ccos-cli` inspection, validation, export and hardware/doctor operations
- curated MIT/permissive C++ foundation dependencies with pinned versions
- optional MIT technical UI/diagnostics/storage extensions (Dear ImGui, ImGuizmo, cpptrace, unordered_dense, date and foonathan/memory)
- optional professional text, image, audio, codec and storage backends (FreeType/HarfBuzz/libass, OpenEXR/OpenImageIO, AVIF/WebP/JXL, RtAudio/RtMidi/DSP, AV1, compression/archive)
- optional OpenTimelineIO and OpenColorIO integration boundaries
- open cultural-media adapters for Internet Archive, Smithsonian Open Access, The Met, Europeana and Library of Congress
- local `llama.cpp`/`llama-server` provider profile built on the existing OpenAI-compatible transport
- API adapters for external media/utility services with bounded network policies
- GoogleTest coverage plus CI, security and hardening workflows

Heavy multimedia integrations remain optional and are discovered from audited local installations where transitive licensing, codec patents or platform backends need release-time review. Lightweight MIT components remain available through CMake `FetchContent` behind explicit options.

## Architecture

`src/core` contains reusable primitives. `src/media`, `src/timeline`, `src/project`, `src/render`, `src/audio`, `src/effects`, `src/text`, `src/plugins` and `src/ai` remain independent from the UI wherever practical. External providers are isolated in `src/api` and preserve item-level rights metadata.

## Build

Requirements:

- CMake 3.24+
- C++20 compiler
- Qt 6.5+
- GoogleTest 1.14+ when tests are enabled
- FFmpeg at runtime for probing, proxy creation and export

Configure with CMake and build the `ccos_editor` target. Enable `CCOS_BUILD_TESTS=ON` for the test suite.

The default configuration keeps heavyweight professional SDKs optional. See:

- `CMakePresets.json`
- `cmake/CCOSMitDependencies.cmake`
- `cmake/CCOSProfessionalDependencies.cmake`
- `cmake/CCOSExtendedOpenSource.cmake`
- `docs/OPEN_SOURCE_COMPONENT_MATRIX.md`
- `docs/AI_LOCAL.md`
- `docs/HARDWARE_ACCELERATION.md`
- `docs/PROFESSIONAL_INTERCHANGE.md`
- `docs/licenses/THIRD_PARTY_LICENSES.md`

## Media derivatives

`ThumbnailGenerator` creates bounded one-frame JPEG thumbnails. `WaveformGenerator` creates bounded PNG audio waveforms using FFmpeg's audio filter graph. Both are cache-aware and reject input/output collisions, invalid dimensions and oversized generated files.

Local media cache keys include a source fingerprint derived from absolute path, file size and modification time. Replacing media at the same pathname therefore creates a different derived-media key in normal filesystem workflows.

## Open media providers

`CulturalMediaApi` exposes a consistent `CreativeMediaItem` model across open cultural collections. Every adapter keeps a source URL and a rights/licensing field; an API being free to use does not make every returned item freely redistributable.

The automation API also exposes `EditorApi::culturalMediaProviders()` so agents can discover authentication mode, supported media classes and the rights-review requirement without hard-coding provider assumptions.

## Design principles

- stable domain model before advanced features
- no blocking media work on the GUI thread for future long-running media services
- exact rational timeline time instead of floating-point frame positions
- optional services and AI; essential editing remains offline-capable
- explicit third-party licensing
- bounded subprocess/network work
- crash-safe project persistence and recovery
- runtime capability detection instead of hard-coded hardware assumptions
- deterministic cache identities for derived media
- tests for domain behavior and integration flows
- all completed work lands on `main`; historical branches must not contain unreconciled completed changes

## License

CCOS source code is MIT-licensed. See `LICENSE`. Third-party dependencies retain their own licenses and are listed in `docs/licenses/THIRD_PARTY_LICENSES.md`.
