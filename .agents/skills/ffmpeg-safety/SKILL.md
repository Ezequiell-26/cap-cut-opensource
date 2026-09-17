# FFmpeg Safety Skill

All FFmpeg/FFprobe execution must use argument lists through `ccos::core::ProcessRunner`.

Never:
- build shell command strings from project/media data;
- wait forever;
- trust media metadata without validation;
- assume an output file exists because FFmpeg returned zero.

Always define:
- executable policy;
- startup timeout;
- total timeout appropriate to the job;
- bounded stdout/stderr;
- cancellation behavior;
- output existence/size validation;
- useful error propagation.

Keep rendering deterministic and preserve user project state when export fails.
