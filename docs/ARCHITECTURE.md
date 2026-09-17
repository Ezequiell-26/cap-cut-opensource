# Architecture

CCOS is intentionally layered.

## Core

`core` owns exact time arithmetic, identifiers, errors and shared primitives. It must not depend on the desktop UI.

## Media

`media` owns asset references, probing, import, decoding and media caches. FFmpeg will be integrated behind interfaces so the rest of the application does not depend directly on FFmpeg types.

## Timeline

The timeline model contains tracks and clips and operates on `core::Time`. Editing commands, snapping, keyframes and linked media will build on this model.

## Project

`.ccos` is the project interchange/persistence format. Media files remain external references; the project stores metadata, timeline state and editor settings.

## Rendering

Preview and export are separate pipelines. Both consume project/timeline state but have different performance goals. Preview favors low latency and caching; export favors deterministic output.

## UI

Qt 6 is an adapter over the application/domain layers. Business logic should not be hidden inside widgets.

## Planned modules

`audio`, `render`, `effects`, `transitions`, `text`, `gpu`, `plugins`, `ai` and `platform/*` will be added as independent modules with explicit interfaces.
