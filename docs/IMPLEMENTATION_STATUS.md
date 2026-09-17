# Implementation status

## Stable implementation

- C++20 + CMake project structure
- Qt 6 desktop application shell
- Media bin and multi-file media import references
- Optional FFprobe metadata probing
- Qt Multimedia source preview playback
- Versioned `.ccos` project persistence (v6)
- Atomic project writes with `QSaveFile`
- Stable project/media/clip/text-layer identifiers during serialization
- Duplicate identity validation on persistence
- Persistent timeline tracks and clips
- Exact rational timeline time in the core domain
- Clip source ranges (`sourceIn/sourceOut`)
- Timeline operations: move, delete, trim, split
- Generic keyframe interpolation with linear/ease modes
- Command stack with undo/redo and failed-redo history preservation
- Autosave/recovery snapshot support
- Bounded filesystem media cache primitive
- FFmpeg proxy generation with bounded external-process execution
- FFmpeg thumbnail and waveform generation with bounded external-process execution
- Render job and render queue domain models
- Asynchronous FFmpeg render executor with progress and cancellation
- Hardware encoder capability probing through the common process runner
- Multitrack video/audio FFmpeg compositor
- Clip transforms: position, scale, rotation, opacity, crop, horizontal/vertical flip
- Clip playback speed and audio tempo compensation
- Built-in effects mapped to FFmpeg filters
- Fade/dissolve clip-in transitions
- Project text layers with persistent styling and FFmpeg `drawtext` rendering
- SRT/VTT subtitle parsing and SRT writing
- Optional local Whisper CLI transcription through bounded process execution
- Plugin manifest scanning and API compatibility checks
- Audio buffer, gain, linear fades and soft-clip DSP primitives
- RenderGraph implementation with cycle detection, deterministic topological ordering and duplicate-ID protection
- Central `ProcessRunner` with startup timeout, total timeout, cancellation and bounded output
- Central `JobSystem` with bounded concurrency, cancellation, pause state and explicit retry policy
- Native C++ repository guard
- C++-only editor/runtime policy
- Agent contract, roles, skills, workflows and policies for deterministic vibe coding
- CMake presets
- GoogleTest suite with reproducible FetchContent fallback
- Cross-platform GitHub Actions configuration for Windows, Linux and macOS
- Sanitizer and CodeQL workflows
- Tagged release packaging workflow with CPack artifacts
- Licensing and third-party dependency documentation

## Current verified product path

Create project → import media → select/preview source media → add clips to timeline → edit clips → undo/redo → save/load `.ccos` → autosave/recovery → export through the asynchronous FFmpeg compositor.

The project intentionally does not count experimental placeholders as production functionality.

## Experimental / not production yet

- `src/preview/pipeline/PreviewPipeline.*` remains experimental and is excluded from the default build until it has a real composed-frame backend.
- `src/render/scheduler/RenderScheduler.*` is retained only as a legacy compatibility implementation and is excluded from the default build; new work must use `JobSystem`.
- Optional OpenCV/MediaPipe vision is disabled by default and requires native dependencies.

## Remaining for a professional NLE target

### Editing / timeline

- Full interactive timeline UI with drag/drop, trim handles, snapping visualization and direct manipulation
- Ripple/roll/slip/slide editorial tools
- Track linking, grouping, markers, range selection and advanced keyboard workflows
- Nested timelines and compound clips

### Preview / playback

- Timeline-composed, frame-accurate preview
- Dedicated decode/cache pipeline
- Audio/video synchronization at the composed timeline level
- Proxy switching and lifecycle UI
- GPU viewport
- Frame stepping and deterministic seeking

### Render / GPU

- Executable render graph backend integration
- GPU frame processing
- Vulkan backend
- Metal backend
- Direct3D backend
- Hardware decode/encode selection throughout the pipeline
- Resumable render queue with persistent history
- Render diagnostics and performance telemetry

### Audio

- Full routing and buses
- Peak/RMS metering
- EQ
- compressor
- limiter
- noise reduction
- professional synchronization and monitoring

### Effects / text

- Live effect inspector
- Keyframe editor UI
- Full transition library
- Font discovery and management
- Stroke/shadow/background text styling
- Animated text and motion graphics

### Color

- Professional color management
- scopes
- HDR workflows
- LUT management
- 10/12-bit processing end-to-end

### Plugins

- Versioned plugin ABI/protocol
- Permission/capability system
- isolated native plugin host
- IPC protocol
- signature/trust policy

### AI

- Structured AI tool schema
- intent validation
- command preview before mutation
- safe project/timeline editing tools
- local model management
- model capability discovery
- AI task cancellation and budgets

### Distribution

- Platform-native installers
- Dependency bundling verification
- Code signing
- Update/recovery channel
- Reproducible release metadata and SBOM

Items remain explicitly unmarked until a real implementation and automated verification exist.
