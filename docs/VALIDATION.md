# Historical validation record — first modernization phase

Recorded on 2026-09-17 for implementation `e28b8d0edaabee77af7f268096be875076aa8d62`. **The results below apply only to that first-phase implementation and its explicitly named runs, not to the current PR head.** Later follow-up commits change production code, the GUI, tests, and packaging; their results must be checked separately.

See [follow-up validation](FOLLOWUP_VALIDATION.md) for the resumed candidate, actual independent review output, executed checks, and any pending platform/CI gates. Historical green runs and old coverage percentages must not be presented as verification of newer code.

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

## Completed remote GUI and cross-platform verification

All seven jobs passed in [run 35212179946](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35212179946), including the final GUI address-column correction. The preceding complete matrix [35211889151](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35211889151) also passed.

| Job | Environment and scope | Actual result |
|---|---|---|
| headless GCC | Ubuntu 24.04, strict C++11 Release | 3/3 CTest targets pass |
| headless Clang | Ubuntu 24.04, strict C++11 Release | 3/3 CTest targets pass |
| headless MSVC | GitHub-hosted Windows, strict Release | 3/3 CTest targets pass |
| Qt | Ubuntu 24.04, Qt5, strict C++11 Debug, offscreen | 5/5 CTest targets pass |
| address-undefined | Ubuntu 24.04, ASan + UBSan, leak detection enabled | 3/3 CTest targets pass; no sanitizer report |
| thread | Ubuntu 24.04, TSan, halt on error enabled | 3/3 CTest targets pass; no race report |
| coverage | Ubuntu 24.04, GCC, gcovr 7.0 | Tests pass; HTML and XML produced |

The Qt run includes **21 legacy GUI test slots** and **5 new GUI test slots**. QTest reports 23 and 7 passes respectively because it also counts initialization and cleanup; those are not 30 distinct GUI scenarios. Both executables report zero failures and zero skipped tests. Windows validation is headless only; the sanitizer jobs do not include Qt.

The `qt-tests` artifact includes a real rendered screenshot. Visual review found a clipped memory-address column in the first screenshot; content-sized identifier columns fixed it. A font-metric assertion now verifies full address visibility, and the updated screenshot was inspected again.

### Measured coverage, with denominator

The `coverage.xml` artifact from run 35212179946 reports:

- Line coverage: **600 / 624 = 96.15%**.
- Branch coverage: **969 / 1485 = 65.25%**.

The configured filter includes `src/`, `include/`, `lexer.cpp`, `parser.cpp`, and `token.cpp`. It excludes tests, third-party Catch, the Qt frontend, and the executable entry-point file. The command uses `--exclude-unreachable-branches`. These are selected core/runtime coverage figures, **not whole-project or GUI coverage**, and not a claim of 95% branch coverage. The measured branch coverage still leaves meaningful room for additional error-path and fault-injection tests.

Artifact: `coverage`, ID `10492083559`, ZIP SHA256 `1ba1021b82509546be8b34a02c1087f9f812e585c2c247652276603579ce6fb1`. CI retains downloadable artifacts for 14 days; the workflow and reproduction commands remain in Git.

## Validation-driven fixes

The first full Qt import attempt [35211508728](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35211508728) stopped on a copied `QString` range-loop variable in a new test under `-Werror`. It was changed to `const QString&`; the warning policy and test were not disabled. The next import [35211738605](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35211738605) built and passed all five CTest targets before publishing the implementation. The one-time importer was then removed; ordinary PR CI now validates the branch.

## Review gates

See [review record](REVIEW.md) for the implementation self-review and explicit remaining limits.

- Syntax and execution support are explicitly different; parser-only opcodes fail at runtime.
- Frontends use acknowledged commands or isolated snapshots, not unsynchronized access to the worker's mutable machine.
- Multi-byte accesses validate their complete range before writing; addresses are not narrowed to 16 bits.
- Signed arithmetic uses wide intermediates; unsigned arithmetic uses unsigned semantics.
- Loading has no hidden first step, Pause does not consume another command, and tests wait for completion rather than fixed delays.
- Original fixtures have explicit public-source provenance; no private course documents are included.
- Passing tests and sanitizer runs are not universal race-freedom or memory-safety proofs.

## Reproduction

Use the exact build/test commands in README. Test process timeouts are configured in CTest and in subprocess tests. Sanitizers are separate builds. GCC coverage requires `gcovr`: configure with `-DMIPS_COVERAGE=ON`, build, then run `cmake --build build --target coverage`.

A same-compiler, same-host parser/execution microbenchmark was run with three samples per workload; see `benchmarks/README.md` and its raw JSON. Results are synthetic and workload-specific, not a general speedup claim. Header self-containment was checked for every `include/mips/*.hpp` with a standalone strict C++11 compilation.
