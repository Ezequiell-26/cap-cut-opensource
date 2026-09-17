# Hardware Validation Workflow

## Goal

Validate export behavior across software and optional hardware encoders without requiring a GPU on CI.

## Sequence

1. Build CCOS with the default dependency profile.
2. Run the GoogleTest suite.
3. Run `ccos_cli --op doctor --ffmpeg <ffmpeg>` on a host with FFmpeg installed.
4. Verify the JSON result contains encoder discovery, vendor flags, and software fallback codecs.
5. On GPU runners, run explicit smoke exports for the detected encoder family.
6. Never fail the generic build because a hardware encoder or driver is unavailable.

## CI policy

- CPU-only CI is authoritative for compile and unit correctness.
- Hardware jobs are additive capability checks.
- Hardware-specific failures must identify the executable, encoder, and driver path involved.
- Do not silently retry a failed hardware export with arbitrary flags.
- Automatic `auto` encoding must remain usable by falling back to `libx264`.

## Required artifacts

Store only bounded logs and small JSON capability snapshots. Do not upload source media, API keys, or private filesystem paths unless a dedicated test explicitly requires them.
