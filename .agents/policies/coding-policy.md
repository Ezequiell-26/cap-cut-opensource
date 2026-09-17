# C++ Coding Policy

CCOS editor/runtime source is C++ only.

Required:
- C++20 or newer where project policy allows;
- RAII and explicit ownership;
- const-correctness;
- bounded resource use;
- thread-safe state where shared;
- clear error propagation;
- no GUI-thread blocking for expensive work.

Avoid global mutable state and hidden side effects.

Do not add another runtime language to the editor to work around a C++ design problem.
