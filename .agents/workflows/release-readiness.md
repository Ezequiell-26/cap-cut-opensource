# Release readiness workflow

Run this workflow before a tagged release or a merge that materially changes rendering, persistence, dependencies, plugins or security boundaries.

## Sequence

1. Read `AGENTS.md` and the relevant skills.
2. Inspect the complete diff against `main`.
3. Run `tools/ccos_guard`.
4. Configure a clean Release build with default options.
5. Build all default targets.
6. Run the full CTest suite.
7. Run the security workflow and dependency/license audit.
8. Run the nightly hardening configuration or an equivalent sanitizer build.
9. Run packaging smoke tests with CPack.
10. Validate that no secrets, user media, build artifacts or generated binaries entered the repository.
11. Record known platform-specific limitations.
12. Only then mark the change release-ready.

## Failure policy

Any build/test/security failure blocks release readiness. Do not mask failures by disabling tests, widening timeouts without justification or removing the failing subsystem from the build.
