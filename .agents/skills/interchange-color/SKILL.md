# Interchange and Color Management

Use this skill for OpenTimelineIO, OpenColorIO, timeline interchange, LUTs, ICC/OCIO configs, HDR metadata and color-space transforms.

## OpenTimelineIO

Treat OTIO input as untrusted structured data. Validate object counts, time values, media references, metadata size, recursion depth and unsupported schemas before converting to CCOS domain objects.

The C++ runtime must not depend on Python plugins. Keep OTIO optional and locally discovered unless a future release process explicitly adds a reproducible SDK bundle.

## OpenColorIO

Use a secured supported OCIO version. Never parse untrusted LUT/config data through older vulnerable releases. Bound file size before parsing and surface parser failures instead of falling back silently.

Color transforms must preserve the declared source/destination color space, bit depth, display transform and HDR metadata. Do not invent color-space information when it is absent.

## Architectural boundary

External interchange -> validated adapter -> CCOS Project/Timeline model -> command/undo layer.

Color management -> media/format metadata -> render graph -> GPU/CPU transform -> export.

Do not put interchange parsing or OCIO calls directly in Qt widgets.

## Verification

Every adapter change needs malformed-input tests, round-trip tests where applicable, and a failure-path test that proves the project remains unchanged after rejected input.
