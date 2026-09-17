# CCOS Project Memory

This document is a persistent product and engineering memory for future human and AI contributors. Read it before making architectural or product decisions.

## Product identity

CCOS is an open-source professional non-linear video editor (NLE). The long-term product target is a serious desktop editing application with professional capabilities comparable in scope and workflow quality to products such as CapCut and DaVinci Resolve.

This is not intended to remain a prototype, demo, toy editor, or AI-generated mockup.

## Language and stack constraint

The editor implementation is **C++ only**.

Allowed core ecosystem:

- C++20+
- Qt 6 for the desktop application/UI
- CMake for the build system
- FFmpeg/FFprobe and other native C/C++ compatible libraries where their licenses and integration are appropriate
- Native platform APIs and SDKs when required

Do not introduce JavaScript, TypeScript, Python, Rust, Go, Java, Kotlin, Swift, or other programming languages as part of the editor implementation.

Scripts used only as external development/CI tooling must never become runtime dependencies of CCOS. When a task can be implemented in C++, prefer C++ rather than adding another runtime language.

## Product quality target

The target is professional editor quality across:

- timeline editing
- frame-accurate preview
- media import and management
- proxy workflows
- audio editing and mixing
- effects and transitions
- text, captions and subtitles
- keyframes and animation
- compositing
- rendering and export
- hardware acceleration
- color management and HDR
- GPU processing
- project persistence and recovery
- plugin extensibility
- local/optional AI assistance
- professional UI/UX
- cross-platform packaging and reliability

Features must be implemented as real functionality, not represented by placeholder UI or claims.

## Architecture principles

- Keep the domain/engine independent from the Qt UI where practical.
- Keep UI code free of core business logic.
- Use explicit interfaces between domain and infrastructure.
- Treat FFmpeg, FFprobe, network APIs, AI providers, plugins and external processes as infrastructure/trust boundaries.
- Use a command system for state-changing editor operations.
- Use a job system for expensive asynchronous work.
- Keep preview and export as separate pipelines with shared domain state but different performance characteristics.
- Preserve stable project, media, clip and text-layer identifiers.
- Preserve `.ccos` backward compatibility through explicit schema versions and migrations.
- Prefer deterministic, reversible operations.
- Fail closed rather than guessing when input or state is unsafe.

## AI / vibe-coding rules

AI is an engineering assistant, not an unrestricted mutation layer.

AI-generated changes must follow:

request -> scope/risk analysis -> implementation -> tests -> security review -> build -> diff review

AI responses must not directly mutate Project/Timeline state. AI actions should become validated structured intents and then validated Commands.

No agent may declare a feature complete without appropriate automated verification.

## Security principles

- Never commit secrets or credentials.
- Never execute shell command strings constructed from untrusted project/media/AI data.
- Never load native plugins merely because a manifest exists.
- Plugin execution should remain deny-by-default and move toward process isolation/IPC.
- External processes require explicit arguments, startup limits, timeout/cancellation behavior and useful diagnostics.
- Untrusted project files, media metadata, subtitles, plugin manifests, network responses and AI outputs must be validated.
- Do not silently repair corrupted persistent data when validation and recovery are safer.

## Development priorities

When choosing what to implement next, prefer foundational reliability in this order:

1. architecture and module boundaries
2. project format and migrations
3. command system
4. job system
5. safe external-process abstraction
6. timeline correctness
7. media pipeline
8. frame-accurate preview
9. render pipeline
10. audio engine
11. effects/text/compositing
12. plugin sandbox
13. AI tooling
14. GPU backends and advanced professional features
15. packaging, signing and additional platforms

## Definition of success

CCOS should be able to grow rapidly through AI-assisted development without turning into an unmaintainable monolith. Every major subsystem must have clear boundaries, tests, diagnostics, recovery paths and documented invariants.

No document may claim that CCOS can literally never break. The engineering goal is to make regressions difficult to introduce, easy to detect, and safe to recover from.
