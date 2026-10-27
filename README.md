# SM83 in Lean: trial (ensaio)

The trial of §4.4 of the proposal: thirteen SM83 opcodes written as bus
programs in Lean 4, driven one M-cycle at a time from C, checked against the
SingleStepTests, with the ADD flag lemma proved by `bv_decide`. Results and
the decisions they support are in [`DECISIONS.md`](DECISIONS.md).

Opcodes: `00` NOP, `04` INC B, `06` LD B,n8, `20` JR NZ,e8, `77` LD (HL),A,
`7E` LD A,(HL), `80` ADD A,B, `C3` JP a16, `C5` PUSH BC, `C6` ADD A,n8,
`C9` RET, `CD` CALL a16, `F1` POP AF. Every other opcode locks the CPU.

## Layout

| Path | Contents |
|---|---|
| `Sm83/Bus.lean` | Bus programs (`Bus`) and the pure interpreter `Bus.run` |
| `Sm83/State.lean` | Registers and flags |
| `Sm83/Exec.lean` | The instruction monad `M` and the opcodes (`exec`, `instr`) |
| `Sm83/Step.lean` | Step-by-step execution (`Cpu.step`) and the request encoding |
| `Sm83/Ffi.lean` | Functions exported to C |
| `Proofs/Spec.lean` | Reference definitions, independent of the model |
| `Proofs/Add.lean` | ADD A,B and ADD A,n8: result, flags, cycles |
| `Proofs/CheckAxioms.lean` | Fails if a theorem uses axioms outside the rule of §5 |
| `c/` | Command-line program: `sst` and `bench` |
| `tools/fetch_sst.py` | Downloads the test files, pinned by commit and SHA-256 |

## Building

Requirements: [elan](https://github.com/leanprover/elan) (it installs the Lean
version in `lean-toolchain`), CMake ≥ 3.20, a C17 compiler, Python 3.
On NixOS, `nix-shell` provides all of them.

```sh
lake build                       # model and proofs
cmake -S . -B build && cmake --build build
python tools/fetch_sst.py        # test files into tests/sst/
```

CMake runs `lake build Sm83` itself, then compiles `c/` with the system C
compiler and links it with the static library Lake produces and the Lean
runtime, using the flags reported by `leanc --print-ldflags`.

## Running the trial criteria

```sh
build/sm83 bench                       # performance: >= 4.2 M M-cycles/s
lake env lean Proofs/CheckAxioms.lean  # proof: theorems and their axioms
build/sm83 sst tests/sst/*.json        # tests: all files must pass
```

`sm83 sst` exits with 0 when every test passes, 1 when some fail and 2 when a
file cannot be read. It compares the final registers (not IME or IE, as the
suite asks), all 64 KB of memory, and every M-cycle: the kind of cycle, and
the address and value of reads and writes. In idle cycles the suite repeats
the last address and value on the bus, a detail of the Ares emulator it was
generated from, so only the kind is compared.
