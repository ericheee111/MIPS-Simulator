# Follow-up validation — resumed 2026-09-18

## Source and scope

This continuation starts from PR #1 / `refactor/simulator-correctness` at
`1da47836c8e4d8547f47e58c3165a18eca892174`. The historical first-phase results in
[VALIDATION.md](VALIDATION.md) are not current-head evidence. Main is not merged
and no release is authorized by this record.

The resumed changes preserve the owning `Token::value()` API, introduce an
explicit lvalue-only `valueRef()` for copy-free inspection, make benchmark
observer ownership and exceptions safe, and deploy the matching Qt offscreen
plugin alongside the native Windows backend. Package checks retain both launch
modes and now require a local Qt plugin configuration and upstream license files.
No historical benchmark CSV or coverage percentage is reclassified as a new
measurement of these changes.

## Executed local checks

Windows, MinGW-w64 GCC 16.1.0, strict C++11 Release, CMake 4.3.3,
Python 3.14.6, Qt disabled:

- 90 Catch test cases / 4586 assertions passed.
- 17 executable CLI integration cases passed.
- All 21 reconstructed fixtures matched their public literals.
- Six packaging-helper cases and five Qt deployment-contract cases passed.
- All 18 controller-benchmark trials completed; failed/rejected observations
  invalidate a trial and are reported, not silently retried or counted as work.
- All six CTest targets passed.

Reproduction: configure with `-DSTRICT=ON -DMIPS_BUILD_BENCHMARKS=ON
-DCMAKE_BUILD_TYPE=Release`, build, then run `ctest --test-dir <build>
--output-on-failure -V`.

The first native run failed with a DLL entry-point error and CLI process faults.
Prepending the selected MinGW compiler's own `bin` directory to the test process
PATH made the **same already-built binaries** pass; Ninja reported no work to do.
The failed and successful local logs were retained separately. This does not
claim that MinGW runtime packaging or native Windows Qt was tested locally.

The Qt deployment-contract tests run the actual install template with a fake
external deploy tool. They cover configuration selection, a relocated prefix
containing spaces, missing plugins, numeric/process-launch failures, and missing
license files. They do **not** test real DLL compatibility or GUI launch; that
requires the separate Windows Qt package job.

## First resumed candidate and independent review

Candidate `cf47fe3ab3188d0d5c64c81870169a9f936f38fc` passed all **12 jobs** in
[run 35305187126](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35305187126),
including native Windows Qt tests, 2x-DPI tests, isolated installed/extracted GUI
package launch, GCC/Clang/MSVC, both sanitizer configurations, fuzzing, static
analysis, coverage, and both headless packages. The earlier failing run
`35251465388` for `1da4783` remains historical failure evidence.

An independent GitHub Copilot review (Lite, 93/94 changed files) was actually
executed in [run 35305234707](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35305234707).
[Review 5244123749](https://github.com/ericheee111/MIPS-Simulator/pull/1#pullrequestreview-5244123749)
recommended changes; its successful workflow status is **not** an approval.
The separate local Codex CLI attempt produced no review because loading its
cloud configuration timed out; it is not counted as reviewer evidence.

Disposition of the three findings:

- **Address fault classification — accepted and fixed.** Address arithmetic and
  `la` address validation now report `FaultCode::Address`; invalid register/source
  operands remain `Operand`, while out-of-bounds memory accesses remain `Memory`.
  Regression cases also verify that a fault does not update registers, memory,
  PC or executed-step count.
- **QString header dependency — accepted and fixed.** The public GUI header now
  explicitly includes `<QString>`.
- **Discarding consumer futures breaks the worker — rejected with executable
  evidence, not silently dismissed.** The worker still owns each promise. A
  future releasing its shared-state reference does not destroy that promise or
  cause `set_value()` to throw `broken_promise`. Additionally, GUI reload already
  waits for a FIFO Pause before clearing stale replies. See the C++ shared-state
  [release/abandon rules](https://eel.is/c++draft/futures.state) and
  [promise rules](https://eel.is/c++draft/futures.promise). A deterministic gate
  test destroys both new and legacy consumer futures before their queued commands
  execute, then verifies Pause, replace and Step still work. A real GUI test
  reloads with an uncollected observation, then verifies the new program's Step.
  No unnecessary cancellation protocol or altered controller semantics were added.

After these changes, local strict C++11 Release validation passes **94 Catch
cases / 4682 assertions**, all 17 CLI cases, the 21 fixtures, packaging and
Qt-deployment contracts, and the 18-trial benchmark (six CTest targets).
Candidate `f0e6793012ec532296a824cef48e03f7782bdd7f` then passed all 12 jobs in
[run 35305822455](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35305822455),
including the new real GUI reload regression. Independent
[review 5244175074](https://github.com/ericheee111/MIPS-Simulator/pull/1#pullrequestreview-5244175074)
(Lite, 94/95 changed files) no longer raised the future-lifetime claim, but correctly
requested `ProgramCounter` classification for invalid taken jump/branch targets.
That additional finding is now fixed without changing branch semantics. Tests
cover `j` and taken/not-taken paths for all six conditional branches, including
failure atomicity; a not-taken invalid target remains unexecuted and is not a fault.

After this last diagnostic fix, local validation passes **95 Catch cases / 4760
assertions** and all six CTest targets. Candidate
`c90cd99f6ca7c008555913e317ab2b1e131af6f6` passed all 12 jobs in
[run 35306176996](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35306176996).
The next independent [review 5244202469](https://github.com/ericheee111/MIPS-Simulator/pull/1#pullrequestreview-5244202469)
(Lite, 94/95 files) raised null-program dereferencing in `step()`.
Default/null construction already returns a sticky `NoProgram` error before
that dereference, so the review's default-construction example is not a crash.
However, moving a valid Machine can leave its source with a null program and its
previous scalar status. An explicit null-program check now handles that actual
edge case; tests cover default/null construction, preserved sticky diagnostics,
a moved-from source, its unaffected destination, and a controller before loading.
Local validation after this guard passes **97 cases / 4807 assertions** and all
six CTest targets.

## Final implementation verification

Validated executable implementation: **`5de4bcd78ad5ef9e5d515e3bcbd232acda2b3d38`**.
[CI run 35306647431](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35306647431)
completed successfully with all **12/12 jobs**. The documentation/evidence update
following that commit changes no executable behavior. Current-head checks remain
visible on [PR #1](https://github.com/ericheee111/MIPS-Simulator/pull/1).

| Check | Executed result for this implementation |
| --- | --- |
| GCC, Clang, MSVC strict builds/tests | Passed; headless CI also builds and runs the benchmark |
| C++ regressions | 97 cases / 4807 assertions |
| CLI / public fixtures | 17 CLI cases; 21 fixtures match their original literals |
| Package helpers / Qt deployment contract | 6 helper cases; 5 contract cases with configuration/failure subcases |
| GUI | Linux Qt; native Windows Qt at scale factors 1 and 2; retained and new regression suites |
| Memory/concurrency diagnostics | Separate ASan+UBSan and TSan jobs passed |
| Static analysis | scan-build job passed |
| Bounded fuzzing | Parser 317,786 runs; machine 904,079 runs; 31 seconds each, no reported crash |
| Filtered core/runtime coverage | Lines 758/772 = 98.19%; branches 1214/1795 = 67.63%; GUI excluded |
| Distribution | Linux and Windows headless archives; Windows Qt archive; isolated installed/extracted launch |
| Independent local package execution | Downloaded Windows headless and Qt archives both passed on the development host, with developer/Qt search paths removed and a fresh working directory |

### Final reviewer disposition

The final implementation was independently reviewed by GitHub Copilot in
[run 35306663279](https://github.com/ericheee111/MIPS-Simulator/actions/runs/35306663279).
[Review 5244235158](https://github.com/ericheee111/MIPS-Simulator/pull/1#pullrequestreview-5244235158)
used Lite effort, examined 94/95 changed files and reported one suppressed
allocation-exception contract comment, not a new executable defect.
That comment is addressed by documenting the actual API in README.md and
parser.hpp: syntax/configured-limit errors use `false`/`error()`, while host
resource failures such as `std::bad_alloc` may propagate, even while formatting
an error. A parse attempt clears the old program and publishes only a complete
new candidate. No unreliable promise of allocation-free diagnostics was added.
The distinct null/moved-from case and all earlier accepted code findings have
regression coverage. The GUI future-lifetime false positive has an
[evidence-backed reply and resolved thread](https://github.com/ericheee111/MIPS-Simulator/pull/1#discussion_r4043766266).

These are real independent AI review outputs with explicit dispositions. They
are not a human audit or a claim that a `COMMENTED` GitHub review is a formal
`APPROVED` event. The original timed-out local Codex invocation is excluded.

### Durable evidence and package identities

The checked-in [machine-readable evidence](validation/2026-09-18-followup-evidence.json)
contains the source commit, exact CI jobs, original review outputs, artifact IDs,
SHA256 hashes, coverage counts, fuzz summaries, PNG dimensions/hashes, and local
package-launch results. Its SHA256 is
`0c91b8753cf0a71fd4f1df2b2c29c4378f79b841bfa82198e1031d26bbc0dfd0`.
The manifest records hashes for outer GitHub artifact ZIPs separately from the
inner distributable CPack archives; they must not be confused.

| Distributable archive | SHA256 |
| --- | --- |
| `MIPS-Simulator-1.1.0-Windows-AMD64-headless.zip` | `5bbbb7c0de90d696a8666bddc9f30c2eb05fd495019d0f5f8970e93381c7af3b` |
| `MIPS-Simulator-1.1.0-Windows-AMD64-gui.zip` | `6db8418d50189610e9193a6c9ca83b6588af7f333aa308a6b17fbd6d13fc805c` |

All three demo stages exist for both Windows scale settings. Initial native
window captures are host-constrained (1028x700 and 1028x750); later normal and
high-DPI captures are 1200x700 and 2400x1400. Their actual dimensions are retained
rather than misreported as uniform. Automated state/font/launch checks and PNG
presence are not manual visual, accessibility, theme, GPU or pristine-OS
certification. Coverage and finite fuzzing are not proofs of absence of defects.

No main-branch merge, version tag, stable release or project-wide license
selection was performed. Existing local audit records remain outside these
commits.
