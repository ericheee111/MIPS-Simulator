# Correctness and maintainability plan

Baseline: `74731fa1af9b1997bcbaa3b01eb88fe7d3486677`.

## Scope and compatibility

Preserve the educational instruction-level model: C++11, optional Qt5 GUI, 32 general registers and HI/LO, byte-addressed little-endian memory, an instruction-index PC, no delay slots or alignment traps, and the CLI commands step/run/break/status/print/quit. Unsupported execution instructions and a PC outside the program must report an error. Loading must not execute a hidden first instruction. Keep the original implementation in Git history and retain meaningful legacy tests.

## Execution sequence

1. Reproduce the unmodified baseline; collect compiler/test outcomes and recover a local source checkout without changing main.
2. Fix tokenization and diagnostics, numeric/label resolution, register and arithmetic semantics, memory bounds, entry point selection and error propagation. Add focused regressions before refactoring interfaces.
3. Separate program representation, assembly, machine state, checked memory and execution control. Preserve useful public compatibility entry points.
4. Make the worker the sole owner of mutable execution state. Commands have completion acknowledgements; pause never consumes another command; EOF and destruction stop and join the worker. CLI and GUI use coherent snapshots.
5. Restore self-contained tests, optional GUI builds, sanitizer configurations and CI. Review changes and record only actually executed results.
6. Publish the reviewed work on an independent branch and open a pull request. Do not force-push or modify main implicitly.

## Validation gates

- Lexing: adjacent comments, line numbers, EOF whitespace, unclosed strings/parentheses and empty strings.
- Parsing: signed/full-width immediates, symbols, duplicate/undefined labels, incomplete statements, nonzero main, re-use after failure, bounded input sizes.
- Execution: zero register, 32-bit wrapping, signed overflow, unsigned HI/LO arithmetic, branch sources, checked loads/stores, negative offsets, no partial write on error.
- Control: ordered steps and snapshots, repeated run/break, run-time errors, quit after break, concurrent callers, idle/run destruction.
- Frontends: argument permutations, missing files, malformed print commands, exact running-state diagnostic, EOF, GUI loading/highlighting and window closure.
- Build: headless C++11 core without Qt, Qt5 tests on Linux, GCC and Clang, ASan/UBSan, and TSan where the runner permits it. Unsupported environments are reported rather than counted as passes.

Performance work follows correctness: two-pass symbol resolution, immutable shared programs, bounded control batches, and avoiding full instruction-vector copies. No speedup or coverage percentage is claimed without measurement.
