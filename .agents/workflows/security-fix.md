# Security Fix Workflow

1. Identify the trust boundary and threat model.
2. Reproduce or model the unsafe behavior.
3. Add a regression test or guard that fails before the fix.
4. Apply a fail-closed fix with minimum scope.
5. Check for equivalent vulnerable patterns elsewhere.
6. Run repository guard, full tests and sanitizers as applicable.
7. Review secrets, permissions, process execution and filesystem behavior.
8. Document the security invariant and residual risk.

Never resolve a security issue by disabling a check or weakening validation.
