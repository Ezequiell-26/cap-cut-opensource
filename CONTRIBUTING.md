# Contributing

## Development rules

1. Keep domain logic out of Qt widgets.
2. Prefer small, reviewable changes.
3. Add tests for new domain behavior.
4. Do not add a dependency without recording its license and purpose.
5. Do not commit generated build output or user project files.
6. Preserve deterministic timeline calculations.
7. Avoid blocking the GUI thread with media operations.

## Commit format

Use Conventional Commit-style messages:

`feat(timeline): add ripple trim`

`fix(media): handle missing probe binary`

`test(project): cover migration`

## Pull requests

A PR should describe the user-visible behavior, testing performed, and any new third-party dependency.
