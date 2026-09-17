# Hardware Export Skill

## Purpose

Safely reason about FFmpeg hardware encoding in CCOS without making hardware support a hard dependency.

## Runtime contract

- Use `ccos::render::HardwareCapabilitiesProbe::detect()` to inspect the configured FFmpeg executable.
- Treat the returned encoder list as runtime capability information, not proof that a particular driver/device is healthy.
- `preferredH264Encoder(true)` may select only encoders supported by the current generic software-frame export path.
- VAAPI is detected and exposed, but must not be selected by the generic automatic path until a dedicated device/upload pipeline exists.
- `preferredH264Encoder(false)` must remain `libx264`.
- `preferredHevcEncoder(false)` must remain `libx265`.

## Agent rules

1. Never hard-code a hardware encoder because the host GPU model is known or guessed.
2. Probe the configured FFmpeg binary at runtime.
3. Keep software fallback available for every automatic selection.
4. Do not add FFmpeg command-line arguments that depend on a hardware device unless the corresponding device discovery and frame-transfer path also exists.
5. Preserve `ProcessRunner` timeouts, output bounds, environment sanitization, and risk classification.
6. Add unit coverage for every new encoder family or selection rule.

## Validation checklist

- FFmpeg executable missing -> capability probe returns an empty set and software fallback remains usable.
- NVENC/QSV/AMF/VideoToolbox available -> automatic H.264 selection can use the corresponding encoder.
- VAAPI available -> report capability, but generic automatic selection falls back to software.
- Explicit user codec -> pass through only after `ExportSettings::validate()`.
