# Implementation status

## Implemented

- C++20 + CMake project structure
- Qt 6 desktop application shell
- Media bin and multi-file media import references
- Optional FFprobe metadata probing
- Qt Multimedia local preview playback
- Versioned `.ccos` project persistence (v6)
- Atomic project writes with `QSaveFile`
- Stable media and clip identifiers
- Persistent timeline tracks and clips
- Exact rational timeline time
- Clip source ranges (`sourceIn/sourceOut`)
- Timeline operations: move, delete, trim, split
- Generic keyframe interpolation with linear/ease modes
- Command stack with undo/redo
- Autosave/recovery snapshot support
- Bounded filesystem media cache primitive
- Real FFmpeg proxy generation
- Real FFmpeg thumbnail and waveform generation services
- Render job and render queue domain models
- Asynchronous FFmpeg render executor with progress and cancellation
- Hardware encoder capability probing (NVENC/QSV/AMF detection)
- Multitrack video/audio FFmpeg compositor
- Clip transforms: position, scale, rotation, opacity, crop, horizontal/vertical flip
- Clip playback speed and audio tempo compensation
- Built-in effects mapped to FFmpeg filters (brightness, contrast, saturation, exposure, grayscale, sepia, blur, sharpen, vignette, invert)
- Fade/dissolve clip-in transitions
- Project text layers with persistent styling and FFmpeg `drawtext` rendering
- SRT/VTT subtitle parsing and SRT writing
- Optional local Whisper CLI transcription adapter
- Plugin manifest scanning and API compatibility checks
- Audio buffer, gain, linear fades and soft-clip DSP primitives
- CMake presets
- GoogleTest suite with reproducible FetchContent fallback
- Cross-platform GitHub Actions configuration for Windows, Linux and macOS
- Tagged release packaging workflow with CPack ZIP/TGZ artifacts
- clang-format and clang-tidy configuration
- Licensing and third-party dependency documentation

## Current application workflow

Create project → import media → select/preview media → add clips to Video 1 → sequential timeline editing → undo/redo → save/load `.ccos` → autosave/recovery → export the complete timeline through the asynchronous FFmpeg compositor.

## Remaining for a full professional NLE release

- Timeline-driven frame-accurate preview of the composed timeline; current interactive preview still uses Qt Multimedia source playback
- Interactive timeline UI with mouse-based trim handles, drag/drop clip placement, snapping visualization and direct manipulation
- Dedicated GPU render backend (Vulkan/Metal/Direct3D) and shader execution; current compositor is FFmpeg-based
- Full xfade-style transitions between adjacent clips; current implemented clip-in fade/dissolve covers the common entry case
- Full audio routing, buses, metering, EQ/compressor/limiter/noise-reduction DSP
- Rich effect inspector with live editable parameters and keyframe UI
- Full text/font discovery, per-font selection, stroke/shadow/background styling and text animation editor
- Automatic proxy switching in preview and proxy lifecycle UI
- Frame/thumbnail/waveform cache integration into the UI
- Smart media relinking and missing-media recovery workflow
- Full hardware decode selection and per-platform acceleration pipelines
- Render queue UI with ETA/retry/history and resumable jobs
- Professional color management, scopes and HDR
- Dynamic plugin loading/ABI boundary and sandbox/security model
- Bundled/local AI model management rather than only the external Whisper CLI adapter
- Platform-native installers, dependency bundling and code signing
- Mobile applications after desktop core stabilization

Items remain explicitly unmarked until a real implementation and automated verification exist.
