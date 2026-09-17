# Reproducible debugger walkthrough

Start with `examples/sum_of_squares.asm`. It computes 1² + ... + 10² = 385.

1. Load the example. PC points at `main`; registers remain zero. Loading is not a step.
2. Click **Step** (F10). Observe the source gutter, next-instruction highlight, and changed-register highlight.
3. Enter `end` next to **Run to**. It pauses *before* executing that instruction, without guessing when to click Break.
4. Enter `4` in the memory address field and press **Go**. Bytes 4–7 are `81 01 00 00` (little-endian 385).
5. Click **Reset** (Ctrl+R) to restore initial data/registers without reparsing. **Reload** rereads the source file.

Run to has a 1,000,000-instruction budget. An unreachable target pauses at the budget;
it does not silently run forever. The UI identifies Target reached / Budget reached /
Running / Paused. **Break** (Shift+F5) remains available during background execution.

Equivalent deterministic CLI session:

```text
until end
print &0x4
print &0x5
reset
print $t0
quit
```

`until LABEL_OR_INDEX [BUDGET]` is a bounded, synchronous CLI convenience; use `run`
and `break` for interactive asynchronous execution. Default budget is 1,000,000,
maximum 100,000,000. It stops before the target instruction and does not add ISA opcodes.

Qt tests can capture the actual loaded/stepped/result states by setting `MIPS_DEMO_DIR`.
The pictures are produced by the application, not mockups. `--gui FILE --smoke-test`
loads, processes a GUI event turn and closes with a status code, for installed-package
startup checks; it is **not** a substitute for the interactive QTest scenarios.

## Packages

`cmake --install build --prefix stage --config Release` installs the executable,
examples and documentation. `cpack --config build/CPackConfig.cmake -C Release`
creates an archive. Runtime-only builds use `-DBUILD_TESTING=OFF` and contain no fault hooks.

Linux GUI archives use system Qt5 libraries; install the platform Qt5 runtime packages.
Windows GUI archives are built with `MIPS_DEPLOY_QT=ON` using windeployqt and upstream
Qt5 license texts. The archive includes Qt DLLs/plugins and the compiler runtime
selected by windeployqt. CI removes Qt/developer paths for the installed application
smoke test. Windows hosted runners still have system runtime components; this is not
a claim of validation on a pristine Windows VM or every desktop theme/GPU.

No project-wide open-source license has been chosen on the owner's behalf. Third-party
notices are preserved; see THIRD_PARTY.md before redistribution.
