# CCOS — Third-party C++ foundation

CCOS uses a curated set of permissive C++ libraries selected for editor infrastructure. The project does **not** assume that every transitive dependency or binary distribution is MIT; every upgrade must be re-audited.

| Project | Pinned version | License / note | Intended CCOS use |
|---|---|---|---|
| nlohmann/json | 3.12.0 | MIT | Structured JSON for services, tooling and automation |
| fmt | 12.2.0 | MIT | Fast type-safe formatting |
| spdlog | 1.17.0 | MIT; depends on fmt | Structured diagnostics/logging backends |
| Taskflow | 4.1.0 | MIT | Task graphs and dependency-aware scheduling |
| magic_enum | 0.9.8 | MIT | Enum reflection for stable diagnostics and serialization helpers |
| cpp-httplib | 0.56.0 | MIT | Optional local HTTP API and automation endpoints |
| GLM | 1.0.3 | MIT or Happy Bunny (modified MIT) depending on source portions | Vector/matrix math for transforms and future GPU/render backends |
| toml++ | 3.4.0 | MIT | Human-editable configuration and future preset files |
| miniaudio | 0.11.25 | Public Domain or MIT-0 | Optional audio-device/backend layer |
| ONNX Runtime | 1.31.0 family | Top-level repository is MIT; distributions/execution providers can add other licenses | Optional local AI inference backend |

## Integration policy

- Versions are pinned in `cmake/CCOSMitDependencies.cmake`.
- The MIT foundation can be disabled with `-DCCOS_ENABLE_MIT_FOUNDATION=OFF`.
- miniaudio is optional because native device backends are platform-sensitive.
- ONNX Runtime is **find-only** when enabled; CCOS does not silently download an inference runtime with unknown execution-provider contents.
- GLM is documented with its actual dual/modified-MIT licensing rather than being represented as a single-license package.
- RmlUi and OpenCut are tracked as reference projects, not copied into the C++ runtime. RmlUi has additional third-party assets, while OpenCut's current application stack includes TypeScript/Rust, which conflicts with CCOS's C++ runtime rule.

## Verified upstream license sources

- nlohmann/json — MIT
- fmt — MIT
- spdlog — MIT, with fmt as a dependency
- Taskflow — MIT
- magic_enum — MIT
- cpp-httplib — MIT
- GLM — MIT / Happy Bunny modified MIT
- toml++ — MIT
- miniaudio — Public Domain or MIT-0
- ONNX Runtime — top-level MIT

When shipping a release, preserve all required upstream copyright/license notices and maintain an auditable third-party inventory.
