# Architecture Change Workflow

Use for module boundaries, public interfaces, persistence format, render graph, concurrency, plugin, or AI execution changes.

1. Read current architecture and ADRs.
2. Map current dependency direction.
3. State the invariant being protected and why the boundary must change.
4. Define compatibility and migration behavior.
5. Update or create an ADR.
6. Implement incrementally without mixing unrelated refactors.
7. Add contract and regression tests.
8. Run the full build matrix relevant to the change.
9. Run sanitizers and security checks when appropriate.
10. Update architecture/status documentation.

A structural change without an explicit rollback or migration strategy is incomplete.
