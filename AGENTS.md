# CCOS Agent Contract

CCOS is a C++20+/Qt 6 professional non-linear video editor. This file is mandatory for human and AI contributors.

## Product target

Build a serious desktop NLE with professional editing, preview, media, audio, effects, text, compositing, render, hardware acceleration, color/HDR, plugins and optional local/cloud AI. The quality target is comparable in scope and workflow quality to products such as CapCut and DaVinci Resolve.

## Language rule

The editor/runtime implementation is C++ only.

Allowed ecosystem:
- C++20+
- Qt 6
- CMake
- FFmpeg/FFprobe and other appropriately licensed native libraries
- Native platform SDKs/APIs

Do not add JavaScript, TypeScript, Python, Rust, Go, Java, Kotlin, Swift or another runtime language to the editor.

## Non-negotiable invariants

1. Never rewrite Git history, force-push, delete `main`, or disable required checks.
2. Never commit secrets, credentials, private keys, tokens, user data, local media, build output or generated binaries.
3. Never execute shell strings assembled from project/media/plugin/AI input. Use argument vectors and `QProcess`.
4. Treat project files, media metadata, subtitles, plugin manifests, network responses and AI output as untrusted input.
5. Preserve stable project/media/clip/text-layer IDs.
6. `.ccos` format changes require explicit schema versioning, migration and regression tests.
7. UI must not block on long media, render, AI, network or filesystem operations.
8. External processes must have startup limits, total timeouts, cancellation and bounded output.
9. High-risk external tools such as FFmpeg must sanitize credential-bearing environment variables unless a documented tool-specific requirement proves otherwise.
10. Plugins are deny-by-default. Manifest discovery never implies code execution.
11. AI never directly mutates Project/Timeline state; it must go through validated intent and Commands.
12. Every mutating operation must be undoable unless explicitly documented as non-editor infrastructure.
13. Every completed feature must have automated verification appropriate to its risk.
14. Prefer deterministic and reversible behavior; fail closed on ambiguous or unsafe input.
15. New dependencies require license, origin, version/revision and purpose documentation.
16. Large professional SDKs must remain optional/local unless a reproducible release bundle is explicitly maintained.
17. OpenColorIO integrations must use the secured supported version range documented by the dependency audit.

## Required development loop

DISCOVER -> SCOPE/RISK -> DESIGN -> IMPLEMENT -> TEST -> SECURITY REVIEW -> BUILD -> DIFF REVIEW -> DOCUMENT

Before editing:
- Read the relevant skill and policy.
- Inspect current `main` and affected files.
- Identify invariants, interfaces and existing tests.

After editing:
- Run the repository guard.
- Run formatting/static checks.
- Configure and build.
- Run CTest.
- Run sanitizers/security checks when affected.
- Review the final diff for unrelated changes, secrets and generated artifacts.

## Architecture boundaries

UI -> Application/Commands/Jobs -> Engine/Domain -> Infrastructure -> OS/FFmpeg/GPU/Network

Interchange -> validated adapter -> Project/Timeline domain model.
Color management -> media/format metadata -> render graph -> CPU/GPU transform -> export.

Do not let widgets become service layers. Keep domain code independent from Qt UI concerns where practical.

## Recovery-first

When uncertain:
- validator over silent repair;
- explicit error over guessing;
- migration over incompatible rewrite;
- sandbox over in-process native plugin execution;
- bounded failure over indefinite waiting;
- verified build over assumption.

## Definition of done

A task is done only when implementation, tests, verification, documentation and known limitations are all explicit. Never claim a feature is complete merely because code exists or a build starts.
