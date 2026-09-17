# CCOS Hardware Acceleration

CCOS treats hardware encoding as a runtime capability, not a mandatory build dependency.

`HardwareCapabilitiesProbe` asks the configured FFmpeg executable for its encoder table and records NVIDIA NVENC, Intel QSV, AMD AMF, Apple VideoToolbox, and VAAPI capability flags. The editor API exposes the result through `hardware_capabilities`, while `ccos-cli --op doctor` provides a headless diagnostic path.

The export setting `videoCodec=auto` resolves to a hardware encoder only when the encoder is discoverable and compatible with the current generic software-frame pipeline. Otherwise CCOS falls back to `libx264`. VAAPI is detected for diagnostics but is deliberately excluded from generic automatic selection until CCOS has a device/upload pipeline for it.

Explicit codec requests continue through `ExportSettings::validate()` before being passed to `ProcessRunner`. Hardware detection therefore cannot bypass command validation, timeout limits, output bounds, environment sanitization, or subprocess risk classification.

## Recovery model

The desktop editor stores autosaves under the application data directory in `Recovery/<project-uuid>.ccos`. Each project gets its own recovery slot, preventing opening or saving another project from deleting an unrelated recovery snapshot. Startup inspects the recovery directory and offers the newest snapshot to the user. A normal save removes the active project's recovery snapshot; unsaved projects remain recoverable after a clean close.

Project persistence continues to use `QSaveFile`, so an autosave is committed atomically rather than by writing directly over the previous snapshot.
