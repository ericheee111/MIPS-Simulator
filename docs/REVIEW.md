# Implementation review

Date: 2026-09-17. PR: [#1](https://github.com/ericheee111/MIPS-Simulator/pull/1).

This is an implementation self-review, not independent third-party approval. Review compared the original baseline, changed code, retained tests, new regressions, completed compiler/sanitizer jobs and actual Qt screenshots. No unresolved blocking finding was identified in that review; tests do not establish absence of all defects.

## Design decisions retained

- C++11 and optional Qt5; no forced language/framework migration.
- Instruction-index PC, byte-addressed little-endian memory, no cycle/pipeline/delay-slot model and no alignment traps.
- 42 parsed mnemonics versus 28 executed mnemonics, with explicit faults for parser-only execution.
- The root token/Parse/VirtualMachine read-and-step compatibility surface remains; old parser internals and Worker/message_queue APIs are intentionally replaced.
- Immutable published Program plus isolated mutable Machine state. The worker exclusively owns its Machine; snapshots copy mutable state and share immutable code.

## Findings resolved and verification

| Area | Resolution | Verification |
|---|---|---|
| Lexing | Adjacent comments flush tokens, consumed newlines advance source positions, EOF validates strings/parentheses and terminates lines consistently | Retained lexer tests; targeted EOF/comment/string regressions; bounded malformed-input samples |
| Assembly | Two-pass symbol resolution, one numeric interpretation, complete-statement validation, correct strings/NUL, duplicate/undefined diagnostics and reset after failed reuse | Parser and semantic regression tests; public fixture consistency |
| Execution | Central zero-register write policy; correct signed/unsigned arithmetic and HI/LO; register/immediate branches; nonzero main entry and sticky faults | Boundary examples; 200 fixed-seed high-unsigned arithmetic samples; legacy instruction expectations |
| Memory | Full-range checks before stores, wide signed address calculation and checked public-IR operands | Nonzero absolute loads, negative offsets, high addresses, out-of-range loads/stores and no-partial-write assertions; ASan/UBSan |
| Concurrency | FIFO completion acknowledgements, sole-owner state, isolated snapshots, idempotent Run/Pause, orderly shutdown with pending commands | Ordered/concurrent producer and shutdown regressions; TSan; no fixed sleeps used as command acknowledgements |
| CLI | String argument parsing, no filename shadowing, bounded command parsing, fresh acknowledged state, EOF/quit join path | 13 executable integration cases and Catch command tests; GCC/Clang/MSVC |
| GUI | No implicit load-step, main-thread rendering, pending-command button state, valid source highlighting, value-owned cursor and stop/join on close | 21 legacy and 5 new Qt test slots; real offscreen screenshot inspection |
| Visual regression | First screenshot exposed clipped memory addresses; identifier columns now size to contents | Font-metric assertion plus inspection of the updated screenshot |
| Build/publication | Qt-free core build, target-scoped warnings, Threads::Threads, opt-in coverage/sanitizers and recovered public fixtures | Seven successful CI jobs; temporary source-import workflow removed after publication |

The only changed original Catch expectation is the success assertion for negative `.space`; its original input remains and an inline comment explains why rejection is correct. Legacy GUI expected values remain; synchronization now waits for command completion. A new-test Qt range-loop warning was fixed rather than weakening `-Werror`.

## Evidence

[Validation record](VALIDATION.md) contains exact environments, counts, denominators and run links. Final code validation [35212179946](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35212179946) passed all seven jobs, including Windows headless, Linux Qt and separate sanitizer configurations. The initial successful matrix [35211889151](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35211889151) provides a second complete run. PR checks validate subsequent documentation commits.

## Remaining limits and tradeoffs

This remains an educational restricted-language interpreter, not a complete MIPS implementation, binary compiler or security sandbox. Decimal literals, ASCII identifiers/strings, division corner-case policy and resource limits are documented. It does not implement the 14 historical parser-only mnemonics or three-operand division at runtime.

Selected core/runtime line coverage is 96.15%, while branch coverage is 65.25%; neither figure includes Qt. Exception/allocation failure injection and broader branch exploration would improve coverage. The fixed-seed tests are not an exhaustive fuzzing campaign. GUI validation ran on Linux/offscreen, not Windows/macOS or every DPI/theme. MSVC validation was headless. No GUI sanitizer pass is claimed.

Snapshots deliberately copy mutable memory to avoid races; clients choosing the maximum 16 MiB memory capacity will pay a larger snapshot/render cost than the default 1024-byte debugger. Command responsiveness is favored over batching throughput. The microbenchmark isolates specific parser/interpreter workloads and does not measure GUI or controller throughput.

Library clients must not retain a mutable alias to a Program they publish through a shared const pointer. Destruction of a controller must not race callers accessing the controller object's lifetime; concurrent methods and concurrent explicit shutdown are separately handled/tested.

No additional project-wide license was selected on the owner's behalf. The vendored Catch license is preserved. No private course documents, personal credentials or generated executables were added to the tracked source tree. Main remains unchanged; the completed work is published on `refactor/simulator-correctness` for review and an explicit merge decision.
