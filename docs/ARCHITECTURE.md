# Architecture and invariants

## Scope

Retain the C++11 instruction-level model and optional Qt5 presentation. Do not turn this maintenance pass into an ISA expansion, JIT, or cycle-accurate emulator. Keep the original implementation in Git history. The root `tokenize`, `Parse`, `VirtualMachine`, `VM_Status`, and read/step APIs remain usable by legacy tests. The old internal `Worker`/`message_queue` protocol and parser state-machine helpers are intentionally removed, not silently advertised as compatible public APIs.

## Frontend and program representation

`token.cpp` holds immutable token values/source lines. `lexer.cpp` has explicit string, comment and parenthesis state, one end-of-line path, EOF validation, and a source-size bound. A string body is never reclassified as an instruction or symbol.

`parser.cpp` is an assembler-style two-pass parser. The first pass lays out data, records typed symbols and source rows, and counts instructions. The second validates operands and resolves labels using the complete symbol table. There is no scanning of the remaining token list for each reference. Duplicate symbols, unexpected tokens, incomplete statements, undefined references, and resource limits have line-based diagnostics. Constants retain literal sign information. Only a completely validated candidate is published as `shared_ptr<const Program>`; a failed parse discards any old program.

`Program` is a builder value containing instructions, initial bytes, labels and entry metadata. Published programs are immutable. Machine constructors defensively copy public Program builders, including shared const views. Retaining and later modifying a builder cannot mutate the Machine's internally owned code. Machine copies have private mutable state and share code; they never expose references to another machine's memory.

`src/program.cpp` is the single instruction descriptor table for mnemonic, grammar form and execution support. Runtime opcode lookup is bounded indexed access, not string dispatch. Its enum/table correspondence is checked before use.

## Execution and errors

`Machine` is deliberately single-threaded. Each step validates the instruction and operands, computes using host types wide enough for the required arithmetic, and commits effects only when their preconditions hold. `Memory` checks the entire interval before any write; address addition is performed in a wide signed type and then range-checked. Invalid public IR register/source/offset values fail safely. Register zero uses a central write-discard path; load or overflow faults are not suppressed merely because the destination is zero.

Errors preserve PC, destination state, and memory for the faulting instruction, apart from the explicit error status/diagnostic. Successful last instructions may leave PC one-past-end; the next attempted fetch faults. A taken out-of-bounds jump/branch faults before changing PC. A machine with no valid main entry starts in error. Step after an error is a no-op. Inspecting memory outside the machine step API throws `out_of_range`; the frontends turn it into a user-facing diagnostic.

No simulated alignment traps or delay slots are added. `la` computes an address without reading its contents, but requires it to lie in configured memory. Division policies and parser-only instructions are explicit in the README.

## Concurrency protocol

`ExecutionController` owns one machine and a C++11 worker thread. The worker exclusively reads/writes mutable execution state. Producers hold a mutex only to append a bounded FIFO command. Every accepted command receives a monotonic sequence number and a promise/future. Its acknowledgement contains the state **after** the operation, not an immediately sampled, potentially stale VM.

Commands: Load, Step, Run, Pause, Observe, Reset and RunUntil. Run is a state transition, not a self-enqueuing message. With an empty queue, execution uses bounded batches of 64 instructions (configurable 1..4096). Commands remain FIFO; after at most 16 consecutive observations, one instruction is interleaved before another observation if still running. A control command at the front is never bypassed. Step while running is rejected. A Pause acknowledgement guarantees that later snapshots are paused unless a later Run command resumes execution. Load pauses and replaces state without an implicit first step. A runtime fault clears running and returns to an idle condition-variable wait.

Shutdown sets a protected stopping flag, rejects new submissions, drains accepted requests, then joins under a separate join mutex. It is idempotent, including simultaneous shutdown calls. It does not drop the next message. Outstanding futures receive either a result or an exception on an unexpected worker failure. Public calls may overlap while the controller lives, but object destruction must not race a caller's access to the object itself.

The modern request API returns a Reply containing coherent registers, PC, status and only an explicitly requested memory window (up to 4096 bytes). Run/Pause acknowledgements need no memory. The legacy full Snapshot API remains for compatibility, and is explicitly expensive. Neither API exposes mutable worker memory. Invalid windows/targets are rejected before command side effects. Reset reuses sealed code but restores all initial state. RunUntil stops before its target or when a finite instruction budget expires.

A worker failure completes current and queued promises exceptionally and marks the controller stopped. Test-only hooks (BeforeCommand/BeforeReply/BeforeAutomaticStep) deterministically exercise this path; they are excluded when BUILD_TESTING=OFF. Failure while producing a reply can happen after a command's effects: an exceptional future is not a rollback guarantee. Shutdown drains commands but schedules no new background batch after seeing stopping; an already executing batch finishes first. Destruction must not race callers accessing the object's lifetime.

## Frontends

The CLI waits on command acknowledgements. Parsing command words never indexes a too-short string; numeric conversions are bounded and checked. EOF, quit and exceptions all use RAII shutdown.

Qt only owns frontend objects on its own thread. Its command completion timer is active only while requests are pending; rendering observations have a separate 33 ms cadence while running. Paused idle frontends stop both timers. A lazy QAbstractTableModel represents logical byte rows while holding only the observed window, without one allocated item per byte. Command submission exceptions and future-result exceptions are both caught at the frontend boundary. A source gutter, changed-register highlights, memory-address navigation, Reset/Reload and bounded Run to augment the existing controls. A source reload first obtains a pause acknowledgement and discards older frontend futures, preventing a stale result from replacing the newly loaded state. File loading is bounded; failure clears the old program and disables execution. Highlighting checks the program, source line, and text block. QTextCursor is a value object. Closing does not depend on receiving a GUI callback to stop the worker.

## Tests and compatibility changes

The original Catch assertions are retained except the incorrect expectation that `.space 70, -20, +90` succeeds. Its input remains unchanged and now asserts failure. The parser test explicitly includes `<sstream>` instead of relying on a transitive include.

Legacy GUI tests retain expected register/memory values. Their click helper waits for the corresponding `commandCompleted` signal; the run test waits for observable output rather than sleeping. Unused local variables are removed for strict compilation. Missing fixture files are reconstructed only from public embedded test programs and verified by a checked-in script. No instructor files or personal course records are republished.

Machine faults also expose Diagnostic with code, source line, PC and (for memory faults) address/width. Legacy Error:<line>: text stays available. Parser diagnostics remain line-based text; no column-precision claim is made.
