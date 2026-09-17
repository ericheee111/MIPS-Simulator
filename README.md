# MIPS Simulator

An educational **instruction-level MIPS assembly simulator and debugger**, implemented in C++11 with an optional Qt5 interface. Originally a 2022 school project, now maintained with checked execution semantics, deterministic thread control, and reproducible tests.

This is an interpreter for a **documented subset**, not a compiler, binary assembler, cycle-accurate CPU model, or complete MIPS implementation. The program counter indexes decoded instructions; there are no pipeline stages, delay slots, coprocessors, or alignment traps.

## Build and test

The default build is headless: **Qt is not required**. Prerequisites: a C++11 compiler, CMake 3.16+, and Python 3 when building tests.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/simmips examples/sum_of_squares.asm
```

On Windows, use `build\Debug\simmips.exe` and `ctest --test-dir build -C Debug --output-on-failure`. A runtime-only build uses `-DBUILD_TESTING=OFF`; this also removes the Python requirement. Project warnings are errors by default (`STRICT=ON`).

### Qt5 debugger

Install Qt5 Widgets and Qt5 Test development packages. For example, on Ubuntu:

```sh
sudo apt-get install qtbase5-dev
cmake -S . -B build-gui -DMIPS_BUILD_GUI=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
./build-gui/simmips --gui examples/sum_of_squares.asm
```

`FILE.asm --gui` also works. CTest configures Qt's offscreen platform for GUI tests; an interactive session uses your normal desktop platform. The historical `Vagrantfile` is retained for context, not as the recommended installation path.

The debugger displays read-only assembly with line numbers, the next instruction's source line, changed-register highlights, and a lazy view over the default 1024 memory bytes. Reset/Reload, memory-address navigation and bounded Run to are available. Loading does **not** execute a hidden instruction. Step waits for a worker acknowledgement; Run executes in the background; Break pauses at an instruction boundary. GUI updates occur only on the GUI thread.

## Command-line debugger

| Command | Behavior |
|---|---|
| `step` | Execute one instruction and print the new instruction-index PC, or an error. |
| `run` | Start continuous execution; repeated Run is idempotent. |
| `break` | Pause after the current instruction; no-op when already paused. |
| `print $t0` | Print a register by alias or number; `$pc`, `$hi`, `$lo` are also supported. |
| `print &0x4` | Print one memory byte; addresses may be decimal or `0x` hexadecimal. |
| `status` | Print the current error, or nothing if there is no error. |
| `reset` | Restore initial registers, memory, PC and clear faults without reparsing. |
| `until end 1000000` | Run synchronously to label/index before executing it, or stop at the instruction budget. |
| `quit` | Stop, join the worker, and exit successfully. EOF also stops and joins. |

While running, `step` and `print` report `Error: simulation running. Type break to halt.` Run `until end` on the included sum-of-squares example, then `print &0x4`: the result 385 is stored little-endian as `81 01 00 00` at addresses 4–7.

Programs conventionally finish in an explicit self-loop (`end: j end`). There is no implicit successful halt instruction. A step past the instruction array is a runtime error; errors are sticky until another program is loaded.

## Supported language and execution

The grammar recognizes 42 mnemonics; **28 have execution semantics**. Parse support is not a claim of execution support.

| Category | Executed instructions |
|---|---|
| Data movement | `lw la sw li move mfhi mflo` |
| Arithmetic | `add addu sub subu mult multu div divu` (two-register division only) |
| Logic | `and nor not or xor` |
| Control | `j beq bne blt ble bgt bge nop` |

The historical parser-only instructions `lh lb sh sb mthi mtlo mul mulo mulou rem remu abs neg negu`, and three-operand `div/divu`, parse but report **unsupported execution instruction** without modifying state when stepped.

Data directives: `.word`, `.half`, `.byte`, `.space`, `.ascii`, `.asciiz`. Memory is little-endian, byte-addressed, and unaligned word accesses are allowed. `.space` accepts a comma-separated list of nonnegative sizes (a retained grammar extension).

Registers use 32-bit bit patterns, and `$zero` always reads zero. Unsigned add/subtract wrap modulo 2^32. Signed add/subtract detect overflow before committing a write. Multiply produces the full HI/LO pair. Relational branches use signed comparison; equality compares bit patterns. Effective addresses are checked without 16-bit truncation and stores validate the complete byte range before writing.

Assembly literals are **decimal**; an explicit `+` or `-` selects the signed range for data-width validation. Constants retain that signedness. Identifiers start with an ASCII letter and contain letters/digits. Empty strings are supported; strings are printable ASCII and backslashes are literal (no escape decoding). Comments begin with `#` outside strings. `.text` alone is valid, and execution begins at `main`, even when other instructions precede it.

For course-undefined division by zero, this simulator deterministically retains HI/LO and advances PC. Signed `INT32_MIN / -1` retains the low 32-bit quotient and a zero remainder, without invoking host-language undefined behavior. These are **simulator policy choices**, not claims of universal hardware behavior.

Limits: 4 MiB source, 100,000 instructions, 1024 bytes default memory. Library clients can select memory capacity with `Parse(bytes)`, up to 16 MiB. Allocation and address failures produce diagnostics, not unchecked host memory accesses.

## Architecture

```text
ASCII source -> Lexer -> two-pass Parser -> shared immutable Program
                                                 |
                                      Machine / checked Memory
                                                 |
                                     ExecutionController worker
                                      /                     \
                                CLI acknowledgements     Qt snapshots
```

The worker alone owns mutable machine state. FIFO commands return futures whose values include the **completed** operation's state and a sequence number. Modern replies contain coherent registers and optional bounded memory windows; legacy full snapshots remain available. Both share only internally sealed immutable code. Public Program constructors defensively copy builders. Pause never removes unrelated messages. Shutdown rejects new requests, drains accepted requests, and joins the worker.

See [architecture and invariants](docs/ARCHITECTURE.md), [migration plan](docs/MODERNIZATION.md), and [validation record](docs/VALIDATION.md).

## Additional verification

```sh
cmake -S . -B build-asan -DMIPS_SANITIZER=address-undefined -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan --parallel 2
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure

cmake -S . -B build-tsan -DMIPS_SANITIZER=thread -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tsan --parallel 2
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-tsan --output-on-failure
```

Sanitizers are separate configurations, not combined with coverage. TSan requires a compatible host/kernel; an infrastructure failure must not be reported as a clean race-detection result.

GCC coverage is opt-in and requires `gcovr`: configure with `-DMIPS_COVERAGE=ON`, build, then `cmake --build build --target coverage`. It produces HTML/XML reports; no unmeasured coverage percentage or performance speedup is claimed.

Legacy VM fixtures were reconstructed from the assembly literals already present in the public test source. `scripts/recover_fixtures.py --check` verifies their provenance byte-for-byte. Legacy GUI assertions remain, with acknowledgement-based waits replacing racy immediate reads; see the validation record for the one corrected negative-allocation assertion.

Third-party Catch2 remains vendored with its original license notice. See [third-party notes](THIRD_PARTY.md).

A bounded optional microbenchmark and same-host measurements are in [benchmarks](benchmarks/README.md). They are explicitly workload-specific, not a resume-wide performance claim.


## Follow-up hardening and distribution

See [the deterministic demo](docs/DEMO.md), [follow-up design/review gates](docs/FOLLOWUP.md)
and [coverage-guided fuzzing](fuzz/README.md). The supported ISA and C++11/Qt5 scope are unchanged.

`cmake --install build --prefix stage --config Release` and CPack provide relocatable
headless archives and an explicit Windows Qt bundle. CI validates Windows Qt at
scale factors 1 and 2, plus package startup with developer/Qt paths removed. Package
startup on a hosted runner is not a pristine-OS certification. Runtime packages are
rebuilt with BUILD_TESTING=OFF, excluding all fault-injection hooks.

Main-branch pushes now run CI. Pushing an explicit version tag starts the same
verification matrix and prepares a **draft** release with SHA256 checksums; it never
publishes a stable release or merges main automatically. CI package artifacts can be
used before a maintainer chooses to tag/release. Project-wide licensing remains the
owner's decision; third-party distribution notices accompany optional Qt bundles.
