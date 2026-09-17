# Professional interchange and color-management layer

CCOS keeps high-cost professional integrations optional so a normal contributor can configure the editor without installing large SDKs.

## OpenTimelineIO

OpenTimelineIO (OTIO) provides a C++ timeline data model and serialization layer suitable for interchange between editorial tools. The current upstream release is v0.18.1. CCOS discovers an audited local C++ installation with `find_package(OpenTimelineIO CONFIG)` and links `OTIO::opentimelineio` when `CCOS_ENABLE_OPENTIMELINEIO=ON`.

The CCOS integration boundary must treat external timeline files as untrusted input. Future adapters must validate:

- clip and track counts before allocation;
- rational time numerators/denominators and duration bounds;
- referenced media paths before opening files;
- metadata sizes and recursion depth;
- unsupported schemas and effects with explicit fallbacks rather than silent corruption.

The integration must not introduce Python as a runtime dependency. OTIO's Python plugins are not part of the CCOS runtime architecture.

## OpenColorIO

OpenColorIO (OCIO) provides professional color-management primitives, LUT/config processing and CPU/GPU transform infrastructure. CCOS discovers it locally with `find_package(OpenColorIO 2.5 CONFIG)` and links `OpenColorIO::OpenColorIO` when `CCOS_ENABLE_OPENCOLORIO=ON`.

CCOS intentionally requires OpenColorIO >= 2.5.2. Upstream v2.5.2 is the security release addressing CVE-2026-42450 in LUT loading and related parser issues. No older 2.5.x build is accepted by the CMake integration.

LUT/config handling remains a trust boundary. Files supplied by projects, plugins or downloaded media sources must be size-limited, parsed with the secured OCIO version and never treated as executable content.

## Build policy

Default builds keep both integrations OFF.

Example configuration after installing audited SDKs:

```text
-DCCOS_ENABLE_OPENTIMELINEIO=ON
-DCCOS_ENABLE_OPENCOLORIO=ON
```

Do not switch these options to automatic `FetchContent` downloads. Professional media/color SDKs have larger dependency graphs and stricter ABI/release management requirements than the small header-oriented foundation libraries.

## Release gate

Before enabling either integration in a distributable build, record the exact package version, compiler/ABI, transitive dependencies and license texts in the release manifest. Re-run the dependency and license audit whenever either SDK changes.
