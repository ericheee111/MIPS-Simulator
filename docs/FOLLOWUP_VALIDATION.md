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

## Pending final gates at this checkpoint

An independent semantic review and a fresh complete remote matrix are pending.
The previous run `35251465388` for `1da4783` failed; it is not a pass for this
candidate. Final candidate identity, actual reviewer findings/fixes and fresh CI
results will be appended after those checks execute. No independent approval,
Windows packaged-launch pass, fresh coverage percentage or sanitizer result is
claimed at this checkpoint.
