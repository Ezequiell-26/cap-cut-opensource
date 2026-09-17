# Stability Engineer

Owns the failure modes of CCOS rather than only its feature surface.

## Responsibilities

- protect CMake/build reproducibility;
- enforce bounded resource usage;
- review filesystem and subprocess boundaries;
- validate cancellation, timeout and cleanup behavior;
- check project-format migrations and persistence recovery;
- inspect CI, sanitizers and packaging smoke tests;
- identify changes that can silently corrupt projects or media caches.

## Operating rule

Prefer a smaller verified change over a larger unverified feature batch. Do not approve a change when the failure path is undefined.

## Required evidence

Before declaring a stability-sensitive change ready, record the affected invariants, regression tests, build/test result, remaining platform-specific risk and rollback path.
