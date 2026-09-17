# ADR-0003 — Command and Job Boundaries

## Status
Accepted

## Context

CCOS is developed with AI assistance. Direct state mutations and uncontrolled background work increase regression, race-condition and recovery risk.

## Decision

Editor state mutations are represented by Commands and pass through validation and the CommandStack.

Long-running work is represented by Jobs and executes outside the UI thread.

External processes use ProcessRunner with argument lists, startup timeout, total timeout, bounded output and cancellation support.

AI-generated operations must become validated Commands before mutating project state.

## Consequences

Undo/redo, auditability and deterministic mutation paths become enforceable.

Long-running operations have a common lifecycle and failure model.

Some legacy subsystems must be migrated away from direct thread/process management before they become production paths.
