# Bugfix Workflow

1. Reproduce the bug or establish why reproduction is currently blocked.
2. Identify the violated invariant and affected subsystem.
3. Add a minimal regression test whenever practical.
4. Fix the root cause, not only the visible symptom.
5. Run targeted tests, then the full CTest suite.
6. Run security/sanitizer checks when the boundary is relevant.
7. Review the diff for unrelated changes.
8. Document the fix and any remaining reproduction limitations.

A bug is not considered fixed without behavioral evidence unless the failure is purely infrastructure/documentation and that is explicitly stated.
