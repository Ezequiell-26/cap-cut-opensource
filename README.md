# CCOS

CCOS (C++ Creative Open Source) is a cross-platform open-source video editor built around a C++ media/timeline core and a Qt 6 desktop UI.

## Current status

The repository is bootstrapped with a real application shell and the first end-to-end domain workflow:

- C++20 + CMake
- Qt 6 desktop UI
- `.ccos` JSON project persistence
- media bin import model
- video/audio timeline tracks
- clip placement model
- rational timeline time type
- unit tests with GoogleTest
- CI configuration

Media decoding, real-time preview, FFmpeg integration and export are the next engineering layers. The current importer stores media references and metadata; it does not pretend to decode unsupported media yet.

## Architecture

`src/core` contains reusable primitives. `src/media`, `src/timeline`, `src/project` and future `src/render`, `src/audio`, `src/effects`, `src/text`, `src/gpu` and `src/ai` modules remain independent from the UI wherever practical.

## Build

Requirements:

- CMake 3.24+
- C++20 compiler
- Qt 6.5+
- GoogleTest 1.14+

Configure with CMake and build the `ccos_editor` target. Enable `CCOS_BUILD_TESTS=ON` for the test suite.

## Design principles

- stable domain model before advanced features
- no blocking media work on the GUI thread
- exact rational timeline time instead of floating-point frame positions
- optional services and AI; essential editing must work offline
- explicit third-party licensing
- tests for domain behavior and integration flows

## License

CCOS source code is MIT-licensed. See `LICENSE`. Third-party dependencies retain their own licenses and are listed in `docs/licenses/THIRD_PARTY_LICENSES.md`.
