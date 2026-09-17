# AI Editing Skill

AI is an intent generator, never an authority over project state.

Required pipeline:

LLM -> structured intent -> schema validation -> permission check -> domain validation -> Command preview -> apply -> undo/redo

AI tools must declare read/write behavior and risk.

Read-only tools may inspect project state.
Mutation tools must validate IDs, time values, paths and parameters.
Destructive operations require explicit confirmation.

Never execute model-produced shell commands or native code.
Never let AI bypass Commands, Jobs, project validation or undo/redo.
