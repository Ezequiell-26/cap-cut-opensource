# Plugin Security Skill

Plugin discovery and plugin execution are separate trust decisions.

Required flow:
manifest -> schema validation -> compatibility -> trust/signature policy -> explicit capabilities -> isolated host/IPC -> execution

Default state is DENIED.

A plugin must never receive implicit:
- arbitrary filesystem access;
- network access;
- process creation;
- project-wide write access;
- credential access.

Native plugin crashes must not take down the editor where process isolation is available.
