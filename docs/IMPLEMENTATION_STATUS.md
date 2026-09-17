# Implementation status

## Implemented

- C++20 + CMake project structure
- Qt 6 desktop application shell
- Media bin and multi-file media import references
- Optional FFprobe metadata probing
- Qt Multimedia local preview playback
- `.ccos` JSON project persistence, versioned to v2
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
- Render job and render queue domain models
- FFmpeg source-media export service
- Single-video-track contiguous FFmpeg timeline export service
- Compositing render-graph abstraction
- Audio buffer and basic gain/mixing primitive
- Extensible effect interface
- Text layer/subtitle-oriented model
- Optional AI provider abstraction
- CMake presets
- GoogleTest suite with reproducible FetchContent fallback
- Cross-platform GitHub Actions configuration for Windows, Linux and macOS
- clang-format and clang-tidy configuration
- Licensing and third-party dependency documentation

## Current application workflow

Create project → import media → select/preview media → add clip to Video 1 → place additional clips sequentially → undo/redo clip insertion → save/load project → recover autosave after unexpected shutdown → export selected source media or a contiguous single-video-track timeline through FFmpeg.

## Still required for a professional NLE release

- GPU render graph backend and shader execution (the graph abstraction exists, actual GPU passes do not)
- Timeline-driven frame-accurate preview using the timeline compositor instead of a single media source
- Multi-track compositing, overlays and adjustment layers
- Full audio mixer with multi-track routing and DSP effects
- Full effect implementations and inspector controls
- Transition compositor
- Text renderer, font discovery and animated text UI
- Thumbnail generator and waveform generation backed by the cache
- Proxy generation and automatic proxy switching
- Hardware decode/encode selection
- Non-blocking render queue execution with progress, ETA, cancel and retry
- Full timeline export with effects, keyframes, transitions and multi-track audio
- Professional color management and HDR path
- Media relinking and missing-media workflow
- Plugin SDK and loading/security policy
- Local AI implementation (for example transcription/captions)
- Installers, code signing and package generation for desktop platforms
- Mobile ports after the desktop core is stable

Items are not marked complete until a real implementation and automated verification exist.
