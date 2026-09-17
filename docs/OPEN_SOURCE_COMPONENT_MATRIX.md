# CCOS Open-Source Component Matrix

This matrix records the planned external building blocks for CCOS. It distinguishes source-code license from API/content rights and from whether a component is fetched automatically.

## MIT / lightweight C++ foundations

| Component | License | Integration | Default | Main use |
|---|---|---|---|---|
| nlohmann/json | MIT | FetchContent | ON | JSON project/API data |
| fmt | MIT | FetchContent | ON | Formatting |
| spdlog | MIT | FetchContent | ON | Logging |
| Taskflow | MIT | FetchContent | ON | Parallel job graphs |
| magic_enum | MIT | FetchContent | ON | Enum reflection |
| cpp-httplib | MIT | FetchContent | ON | Local automation HTTP |
| GLM | MIT | FetchContent | ON | Math/graphics |
| toml++ | MIT | FetchContent | ON | Configuration |
| EnTT | MIT | FetchContent | OFF | Data-oriented 3D/runtime structures |
| meshoptimizer | MIT | FetchContent | OFF | Geometry optimization |
| TinyGLTF | MIT | FetchContent | OFF | glTF/GLB 3D asset inspection/import foundation |
| cxxopts | MIT | FetchContent | OFF | CLI tooling |
| doctest | MIT | FetchContent | OFF | Supplementary tests |
| pugixml | MIT | FetchContent | OFF | XML/interchange tooling |
| miniaudio | Public Domain / MIT-0 | FetchContent | OFF | Audio-device backend |
| ONNX Runtime | MIT at repo level | Local audited install | OFF | Local ML inference |
| Dear ImGui | MIT | FetchContent | OFF | Technical/diagnostic tools |
| ImGuizmo | MIT | FetchContent | OFF | Transform/editor gizmos |
| cpptrace | MIT* | FetchContent | OFF | Crash/stack diagnostics |
| unordered_dense | MIT | FetchContent | OFF | High-performance hash maps |
| date | MIT | FetchContent | OFF | Advanced date/time |
| foonathan/memory | MIT | FetchContent | OFF | Memory allocators |
| libdeflate | MIT | FetchContent | OFF | Fast DEFLATE compression |

`cpptrace` can pull platform/backend dependencies that introduce additional notices or license obligations. Treat the top-level MIT license as insufficient for release approval without checking the selected backend.

## Professional/permissive media stack

| Component | License family | Integration | Default | Main use |
|---|---|---|---|---|
| FreeType | FTL / GPL | Local audited install | OFF | Font rasterization |
| HarfBuzz | MIT | Local audited install | OFF | Unicode text shaping |
| libass | ISC | Local audited install | OFF | ASS/SSA subtitles |
| OpenEXR | BSD-3-Clause | Local audited install | OFF | HDR/VFX image exchange |
| OpenImageIO | Apache-2.0 | Local audited install | OFF | VFX image I/O |
| libavif | BSD-style + codec dependencies | Local audited install | OFF | AVIF |
| libwebp | BSD-style + patent grant | Local audited install | OFF | WebP |
| libjxl | BSD-3-Clause + patent terms | Local audited install | OFF | JPEG XL |
| RtAudio | Permissive, review notice | Local audited install | OFF | Real-time audio I/O |
| RtMidi | Permissive, review notice | Local audited install | OFF | MIDI |
| libsamplerate | BSD-2-Clause | Local audited install | OFF | Audio resampling |
| SpeexDSP | BSD-3-Clause | Local audited install | OFF | Audio DSP |
| RNNoise | BSD-3-Clause | Local audited install | OFF | Noise suppression |
| KissFFT | Revised BSD | Local audited install | OFF | FFT/spectrum tools |
| dav1d | BSD-2-Clause | Local audited install | OFF | AV1 decoding |
| SVT-AV1 | BSD-3-Clause Clear + AOM patent license | Local audited install | OFF | AV1 encoding |
| Zstandard | BSD | Local audited install | OFF | Cache/project compression |
| LZ4 | BSD-2-Clause | Local audited install | OFF | Fast compression |
| xxHash | BSD-2-Clause | Local audited install | OFF | Non-cryptographic hashing |
| libarchive | BSD-like / multi-license tree | Local audited install | OFF | Archive handling |
| libzip | BSD-3-Clause | Local audited install | OFF | ZIP project bundles |
| OpenTimelineIO | Modified Apache-2.0 | Local audited install | OFF | Timeline interchange |
| OpenColorIO | BSD-3-Clause | Local audited install | OFF | Color management |

## API providers

| Provider | Authentication | CCOS adapter | Content/right handling |
|---|---|---|---|
| Openverse | None / provider policy | Existing | Preserve item license metadata |
| Wikimedia Commons | None / provider policy | Existing | Preserve page/creator/license metadata |
| Freesound | Token | Existing | Preserve license; preview intentionally bounded |
| NASA APOD | DEMO_KEY or user key | Existing | Verify item-specific rights/trademark/third-party content |
| Pexels | API key | Existing | Follow provider/asset terms |
| Pixabay | API key | Existing | Follow provider/asset terms |
| Unsplash | Access key | Existing | Follow provider/attribution terms |
| Internet Archive | None | New | Verify item-level rights statement |
| Smithsonian Open Access | API key | New | Verify record/media rights; CC0 applies only where stated |
| The Met | None | New | Prefer public-domain/CC0 records and retain object URL |
| Europeana | API key | New | Preserve `rights` and source/preview URLs |
| Library of Congress | None | New | Verify the authoritative item rights statement |
| Open-Meteo | None | Existing | Data service, not media licensing |
| Open-Meteo Geocoding | None | Existing | Location lookup; data/service terms apply |
| LibreTranslate | Deployment-specific | Existing | Text service |

## Local AI

| Component | Model/runtime | Integration | Default |
|---|---|---|---|
| OpenAI-compatible | Ollama/vLLM/other compatible servers | Existing | Available |
| llama.cpp `llama-server` | Local GGUF/compatible models | `LlamaCppProvider` profile | ON |
| whisper.cpp | Local speech recognition | Existing | Available |
| ONNX Runtime | Local model inference | Existing CMake boundary | OFF |
| Hugging Face | Hosted inference | Existing | User configured |

## Release rules

1. Do not label a component MIT unless its exact version and license text support that classification.
2. Do not treat a free API as a license to redistribute its returned media.
3. Keep heavy multimedia stacks optional unless they are available as audited system dependencies.
4. Pin versions for reproducible builds.
5. Record transitive dependencies and patent/licensing notices at release time.
6. Keep API keys outside projects, logs and crash reports.
7. Preserve creator, license, attribution and source URLs when importing external media.
8. Every completed integration must be present on `main`; feature branches must not contain unreconciled completed work.
