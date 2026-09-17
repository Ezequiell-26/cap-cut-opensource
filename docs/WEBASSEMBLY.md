# CCOS WebAssembly

CCOS now has a first-class WebAssembly build that reuses the existing C++20/Qt editor, rather than maintaining a separate JavaScript editor.

## Targets

- Windows: native Qt Widgets application with the full host media/export stack.
- Web: the same C++/Qt UI and shared project/timeline/media logic compiled with Emscripten and executed inside the browser sandbox.

## Build

Install a Qt WebAssembly kit matching the project Qt version and use the `web` CMake preset. The preset expects Qt to expose `QT_ROOT_DIR` and uses the Qt-provided `qt.toolchain.cmake`.

```bash
cmake --preset web
cmake --build --preset web
```

The GitHub Actions workflow `.github/workflows/webassembly.yml` performs the same build with Qt WebAssembly and publishes a short-lived `ccos-webassembly-*` artifact.

## Browser-compatible scope

The WebAssembly target keeps the shared C++ systems for:

- project data and serialization
- timeline editing commands and keyframes
- media asset models/import flow
- text and subtitle parsing/shaping where compiled
- API adapters using browser-compatible HTTP/CORS paths
- AI provider interfaces and remote OpenAI-compatible endpoints
- UI, media-bin, timeline and inspector
- Qt Multimedia browser playback where supported by the installed Qt WebAssembly multimedia backend

## Native-only scope

The browser build disables host-only integrations that cannot execute inside the browser sandbox, including:

- CLI target
- local automation server
- ONNX Runtime integration
- OpenCV/MediaPipe integration
- native render scheduler prototype
- OpenTimelineIO/OpenColorIO optional host packages
- native audio-device helper libraries
- native professional codec/storage backends
- llama.cpp local server execution profile
- native hardware encoder discovery/use

The existing FFmpeg export pipeline remains the Windows/native path. A future browser export backend should use browser-native media APIs or a dedicated WebAssembly media pipeline rather than attempting to spawn a native FFmpeg executable.

## Media and persistence

Browser builds must treat the filesystem as sandboxed. Project persistence and derived-media caching should use a browser storage adapter (for example, IndexedDB/Origin Private File System) instead of assuming unrestricted host filesystem access. The current port deliberately keeps native filesystem semantics isolated so this adapter can be added without changing the project/timeline model.

## Deployment

The WebAssembly output is static content. Vercel can serve the generated `index.html`, JavaScript, WebAssembly and Qt loader assets directly. The web deployment must use the WebAssembly build artifact; it must not attempt to compile the native Windows target in the Vercel build environment.

## Browser support

Qt documents WebAssembly support for current desktop Chrome, Edge, Firefox and Safari configurations, with some Qt modules and mobile-browser capabilities having additional limitations. CCOS therefore treats the Web build as a separate target profile while keeping the C++ application core shared.
