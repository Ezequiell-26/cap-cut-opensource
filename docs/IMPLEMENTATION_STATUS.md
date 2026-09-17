# Implementation status

## Implemented

- C++20 + CMake project structure
- Qt 6 desktop application shell
- Media bin and multi-file import references
- Optional FFprobe metadata extraction
- Qt Multimedia local preview playback
- Project save/load using `.ccos`
- Atomic project writes with `QSaveFile`
- Persistent media IDs
- Persistent timeline tracks and clips
- Exact rational timeline time
- Clip source ranges (`sourceIn/sourceOut`)
- Timeline operations: move, delete, trim, split
- Generic keyframe interpolation
- Command stack with undo/redo
- Render job and render queue domain models
- FFmpeg source-file export service
- Audio buffer and basic gain/mixing primitive
- Effect interface
- Text layer/subtitle-oriented model
- Optional AI provider abstraction
- CMake presets
- GoogleTest suite
- Cross-platform CI configuration for Windows, Linux and macOS
- Formatting and static-analysis configuration
- Licensing documentation

## Still required for a professional NLE release

- GPU render graph and shader pipeline
- Timeline compositor capable of rendering many tracks/layers
- Frame-accurate timeline playback driven by the timeline model rather than a single source file
- Full multi-track audio mixer and DSP effects
- Full effect implementations and effect UI
- Transition compositor
- Text renderer with font discovery and animated text UI
- Proxy generation/management
- Thumbnail and waveform cache implementation
- Hardware decode/encode selection
- Render queue execution with progress, cancellation and recovery
- Full timeline export that respects clips, trims, effects, keyframes, transitions and audio
- Professional color-management/HDR path
- Autosave and crash-recovery orchestration
- Plugin SDK and sandboxing policy
- Local AI implementations such as transcription/captions
- Installer/signing/package generation for each desktop platform

The repository intentionally does not mark these items as complete until they have real implementations and tests.
