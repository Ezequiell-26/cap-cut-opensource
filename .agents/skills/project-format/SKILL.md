# Project Format Skill

Treat `.ccos` as a public persistence contract.

Before changing serialization:
- inspect current schema/version;
- locate migrations and compatibility tests;
- preserve stable IDs;
- define behavior for unknown/missing fields.

Required verification:
- current-version load/save;
- previous-version migration;
- roundtrip semantic equality;
- malformed/corrupt input rejection;
- atomic write behavior.

Never silently rewrite an incompatible project.
