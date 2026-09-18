# Coverage-guided fuzzing

These are libFuzzer targets, not fixed-seed random unit tests. They never open files,
execute subprocesses or run an assembly program without an instruction budget.

```sh
CC=clang CXX=clang++ cmake -S . -B build-fuzz -DBUILD_TESTING=OFF \
  -DMIPS_BUILD_FUZZERS=ON -DMIPS_SANITIZER=address-undefined -DCMAKE_BUILD_TYPE=Debug
cmake --build build-fuzz --parallel 2
mkdir -p corpus-parser corpus-machine
cp fuzz/corpus/* corpus-parser/
./build-fuzz/parser_fuzzer corpus-parser -max_total_time=60 -timeout=2 -max_len=8192 -rss_limit_mb=2048
./build-fuzz/machine_fuzzer corpus-machine -max_total_time=60 -timeout=2 -max_len=256 -rss_limit_mb=2048
```

CI runs both for 30 seconds with sanitizer failures fatal. Crash inputs and raw logs
are archived. A completed smoke campaign is not exhaustive validation. Minimized
findings belong in deterministic regression tests before a fix is accepted.

Parser target: arbitrary bytes, bounded execution, zero-register/no-partial-fault
invariants, parser reuse. Machine target: arbitrary public IR, high-bit registers,
invalid opcodes/registers/offsets and targets, no partial mutation on fault.
