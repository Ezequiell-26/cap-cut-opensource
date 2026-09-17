# Dependency Audit

Use this skill before adding, removing or upgrading any third-party library, API SDK or codec component.

## Required evidence

For each dependency record:

- upstream repository;
- exact release/tag/commit;
- SPDX license and bundled license text;
- transitive dependencies;
- CMake target and integration mechanism;
- ABI/compiler requirements;
- security advisories affecting the selected version;
- whether it is fetched automatically or discovered locally.

## Preferred integration order

1. Existing system package with a stable CMake config.
2. Pinned FetchContent archive or immutable commit for small, well-audited dependencies.
3. Vendoring only when reproducibility or platform support requires it.

Do not use floating branches such as `main`, `master`, `develop`, `trunk` or `HEAD` in release dependencies.

## CCOS policy

- Keep the default build small and reproducible.
- Make large professional SDKs optional and discover them locally.
- Do not confuse “free API” with permissive content licensing.
- Never silently replace one dependency with another because an AI agent guessed the target or API name.
- Update `docs/licenses/THIRD_PARTY_LICENSES.md` in the same change.
- Add a smoke test or compile-time target check when the dependency is optional.
