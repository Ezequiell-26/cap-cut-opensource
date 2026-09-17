# Dependency upgrade workflow

Use for every third-party version change, including transitive codec/GPU/runtime updates.

## Sequence

1. Identify the exact upstream release and security advisories.
2. Confirm license and any changed license files.
3. Inspect API/ABI changes and CMake target changes.
4. Update the pinned version and `docs/licenses/THIRD_PARTY_LICENSES.md` together.
5. Keep large dependencies optional unless the project has a reproducible release bundle.
6. Add or update a compile/feature smoke test.
7. Run the repository guard.
8. Run a clean build and full tests on all supported desktop platforms.
9. Run sanitizers when the dependency is native C/C++ parsing, media, GPU or memory-sensitive code.
10. Review the final dependency graph and release artifacts.

## Prohibited shortcuts

- floating Git branches;
- unreviewed AI-generated replacement libraries;
- silently changing license assumptions;
- disabling unrelated tests to get a green build;
- shipping a security-vulnerable version merely because it has a convenient API.
