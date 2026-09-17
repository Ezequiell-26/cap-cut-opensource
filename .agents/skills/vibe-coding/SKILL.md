# Vibe Coding Skill

Use this skill for every AI-assisted change.

1. Read `AGENTS.md` and `docs/PROJECT_MEMORY.md`.
2. Inspect current files on `main`; do not assume stale structure.
3. Classify the change and its risk.
4. Identify invariants and affected tests before coding.
5. Prefer small atomic changes.
6. Never invent missing APIs or dependencies.
7. Never replace working code with placeholders.
8. Make state mutations through domain Commands.
9. Make expensive work cancellable through Jobs.
10. Validate all external input.
11. Run guard, build, tests and applicable security checks.
12. Review the complete diff before declaring success.
13. Report what was not verified.
