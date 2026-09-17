# Resource Budgets

Use this skill for import, decode, cache, waveform, thumbnail, preview, render, export, AI and subprocess work.

## Rules

1. Every potentially unbounded input gets an explicit size, count, duration, recursion or allocation bound.
2. Never convert external media dimensions directly into large allocations without checking width × height and pixel format bounds.
3. Never use an unlimited `QProcess` wait. Set startup timeout, execution timeout, cancellation and bounded stdout/stderr.
4. For disk caches, constrain the cache root, sanitize generated filenames/extensions and use crash-safe replacement for final files.
5. Keep resource limits explicit in configuration structs rather than hidden magic numbers in deep rendering code.
6. Large operations must be cancellable and must report a deterministic failure reason when a budget is exceeded.
7. Tests must cover boundary values, zero/negative inputs, overflow-prone values and cancellation/timeout behavior.

## Review checklist

- [ ] memory budget
- [ ] file-size budget
- [ ] item-count budget
- [ ] duration/frame budget
- [ ] process timeout
- [ ] output capture limit
- [ ] cancellation path
- [ ] cleanup after failure
- [ ] regression test
