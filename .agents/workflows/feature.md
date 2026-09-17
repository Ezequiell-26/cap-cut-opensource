# Feature Workflow

1. Read `AGENTS.md`, `docs/PROJECT_MEMORY.md` and relevant skills.
2. Inspect the current implementation and tests.
3. Define affected modules and risk.
4. Prefer existing interfaces; introduce a new interface only when it reduces coupling.
5. Implement the smallest complete change.
6. Add unit/regression tests with the feature.
7. Run repository guard and formatting checks.
8. Build and run CTest.
9. Run security/sanitizer checks for affected trust boundaries.
10. Review the complete diff and update documentation/status.

Never merge an unverified placeholder as a completed feature.
