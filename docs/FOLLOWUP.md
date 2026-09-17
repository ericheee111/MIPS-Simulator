# Follow-up engineering plan and acceptance gates

Scope: build on PR #1 / `9e48c8a983e7fb13f5147d26b5a7e8c2f4c1607c`, without ISA expansion,
JIT, language migration or an implicit main merge. Original review records describe
their dated baseline; this document records the next phase.

## Implemented design

- Deterministic failure-injection tests cover queue capacity, worker command/reply/
  automatic-step failures, accepted-future completion and safe joining.
- Frontends contain both submission failures and future-result failures.
- Coherent light replies separate command acknowledgements from optional bounded
  memory observations. Legacy full snapshots stay available for compatibility.
- FIFO control ordering is retained. An observation burst limit prevents continuous
  queries from starving background execution; empty-queue execution is batched.
- Public Program inputs are defensively copied; Machine copies/reset share only the
  internally sealed code. Structured machine diagnostics supplement legacy text.
- GUI: lazy memory model, command-vs-render timers, idle quiescence, source line gutter,
  changed registers, address navigation, Reset/Reload, bounded Run to, and shortcuts.
- Two real libFuzzer targets exercise parsing/limited execution and arbitrary public IR.
- Main-push CI, static analysis, Windows Qt/high-DPI tests, install/CPack smoke checks,
  and an explicit-tag draft-release workflow complete the engineering workflow.

## Acceptance and review status

Before final handoff, record the exact candidate commit, actual CI runs, independent
reviewer identity/output, findings and subsequent fixes in FOLLOWUP_VALIDATION.md.
A requested review is not a completed review. A green static analyzer is not a human
or AI semantic review. No independent approval is claimed in this planning document.

Required checks: retained tests, new deterministic fault/fairness tests, separate
ASan/UBSan and TSan, fuzz campaigns, Windows/Linux frontend/package tests, complete
normal/high-DPI demo screenshots, explicit source and evidence hashes. Any unexecuted
platform or infrastructure failure must be called out rather than counted as a pass.

## Remaining deliberate limits

The GUI still parses bounded source synchronously during Load. Whole-source column
spans and arbitrary breakpoints are not introduced: Run to covers the deterministic
demo need. Source editing, full ISA execution, Qt6/Rust and pipeline emulation remain
out of scope. Test hooks are never part of runtime-only packages. Windows hosted
runner package tests remove developer paths but do not replace pristine-OS or manual
GPU/theme/accessibility certification. No new project-wide license or stable release
is chosen implicitly. The release workflow only prepares a draft after a version tag.
