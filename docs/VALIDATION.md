# Validation record

## Baseline

Original commit: `74731fa1af9b1997bcbaa3b01eb88fe7d3486677`.

GitHub Actions run [35203252902](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35203252902) built the unmodified source on Ubuntu 24.04 with Qt5. Configure/build exited 0; CTest exited 8. Core unit tests passed. GUI tests reported missing assembly fixtures, 21 failures and a segmentation fault. That failed baseline is retained, not reclassified as a new regression.

An independent local build of the original core on Debian/GCC 14.2.0 passed **38 Catch test cases / 506 assertions**.

## Local implementation verification

C++11, Qt disabled, project warnings treated as errors:

| Configuration | Actual result |
|---|---|
| GCC 14.2.0 Debug | 70 Catch cases / 4444 assertions; all 3 CTest targets pass |
| Clang 17.0.0 Release | All 3 CTest targets pass |
| GCC ASan + UBSan, leak detection enabled | All 3 CTest targets pass, no sanitizer report |
| GCC TSan, halt on error enabled | All 3 CTest targets pass, no race report |

CTest targets: core/controller/CLI Catch tests, 13 real-executable Python CLI integration cases, and consistency checks for all 21 reconstructed public fixtures. The property tests use fixed seeds, including 200 high-unsigned arithmetic samples and 1000 bounded malformed-input samples. These are regression/property checks, not a claim of exhaustive fuzzing or formal verification.

The original negative `.space` assertion was corrected with an inline rationale; the other original Catch expectations remain. GUI immediate reads were replaced with acknowledgement-based waits without changing expected results.

## Remote GUI and cross-platform verification

Qt5 and Windows cannot be exercised in the local headless Linux environment. CI contains strict Linux/Windows headless builds, Qt5 legacy/new GUI tests, separate ASan/UBSan and TSan jobs, and measured coverage. Results are pending until a corresponding run completes; the presence of a job is not evidence that it passed. This section will be updated after remote validation.

## Review gates

- Syntax and execution support are explicitly different; parser-only opcodes fail at runtime.
- No raw thread escapes a lifetime; all VM access from frontends is acknowledged or copied.
- No unchecked multi-byte memory access or 16-bit address parameter remains.
- Signed arithmetic uses wide intermediates; unsigned arithmetic uses unsigned semantics.
- No hidden first step, stale load result, swallowed quit, or fixed-delay test synchronization.
- Original fixtures have explicit public-source provenance; no private course documents are included.
- No performance multiplier, coverage threshold, or universal race-freedom claim is inferred from test counts.

## Reproduction

Use the exact build/test commands in README. Test process timeouts are configured in CTest and in subprocess tests. Sanitizers are separate builds. Coverage is opt-in and reports the selected core/runtime sources; it is not equivalent to a whole-project or GUI coverage percentage.

A same-compiler, same-host parser/execution microbenchmark was run with three samples per workload; see `benchmarks/README.md` and its raw JSON. Header self-containment was checked for every `include/mips/*.hpp` with a standalone strict C++11 compilation.
