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
The new GUI case still requires the revised remote Qt runs. A fresh independent
review and complete CI matrix must be checked for the revised source candidate;
the first candidate's green run must not be substituted for those results.
