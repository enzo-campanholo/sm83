# Decisions

## 2026-10-27: Trial (ensaio), first results

Status of the four criteria of table 5 of the proposal. Measured on a
development machine, **not the reference machine**, which has not been chosen
yet (§12).

Development machine: AMD Ryzen 5 9600X, 32 GB, NixOS (Linux 6.18), GCC 16.2.0,
CMake 4.4.4, Lean 4.35.0.

| Criterion | Result |
|---|---|
| Performance | **Pass here, to be repeated on the reference machine.** 44–46 M M-cycles/s (about 42× the DMG; the criterion is 4.2 M, 4×), three runs of 10⁸ M-cycles, same checksum in every run. |
| Proof | **Pass.** `add8_spec`, `add_a_b` and `add_a_n8` (`Proofs/Add.lean`) are accepted; `Proofs/CheckAxioms.lean` lists only `propext`, `Classical.choice`, `Quot.sound` and five `._native.bv_decide.` axioms. |
| Tests | **Pass.** `sm83 sst` passes 13 000 of 13 000 cases (13 files × 1000) through the command line. |
| Windows | **Not tested.** No Windows machine was available. Next step for Enzo. |

### Checks that the evidence can fail

A defect inserted by hand (carry of ADD computed as `r ≤ a` instead of `r < a`)
was caught by both:

- `bv_decide` failed `add8_spec` with a counterexample;
- the tests failed 5 of the 1000 cases of `80.json`, the ones with B = 0.

That 0.5% input sensitivity is a preview of what §7.2 will measure.

An opcode outside the trial (`01.json`) fails all 1000 cases with "CPU locked"
and exit status 1.

### Decisions

1. **Lean 4.35.0** pinned in `lean-toolchain` (latest stable, 2026-10-12). It is
   past 4.29, so `bv_decide` axioms are named per use, as §5 expects.

2. **Instruction monad in continuation-passing style.** `M` was first
   `StateT State Bus`: 11.5–11.7 M M-cycles/s. Each bind rebuilt the rest of
   the bus program, so every M-cycle paid once per level of nesting. With
   `M α := State → (α → State → Bus State) → Bus State`, the instruction code
   is unchanged (same `do` blocks) and the rate rose to 44–46 M M-cycles/s,
   with the same checksum. Proofs unfold it just as well.

3. **Opcode dispatch with a plain `match`.** Lean compiles it to a chain of
   comparisons. A test with all 256 cases, the 13 real ones last, measured
   12.1 M M-cycles/s against 11.6 M on the version with 13 cases (both before
   decision 2): no cost, as GCC turns the chain into a jump table. This should
   be checked again with the compiler used on Windows.

4. **Fetch convention.** An instruction's program starts with its own opcode
   fetch, as in the SingleStepTests and `Opcodes.json`. On hardware the fetch
   overlaps the previous instruction's last M-cycle; at M-cycle precision the
   bus sequence is the same.

5. **Step interface.** C owns the `Cpu` object. `sm83_step(cpu, data, irq)`
   consumes it and returns the next one; the current request is read with
   `sm83_request` and packed in a `uint32_t` (`Sm83/Step.lean`). Lean no
   longer accepts borrowed parameters on exported functions, so the getters
   consume their argument; `c/sm83.h` wraps them with `lean_inc_ref`.

6. **Build: CMake drives Lake.** CMake runs `lake build Sm83`, compiles the C
   with the system compiler and takes the link flags from
   `leanc --print-ldflags`. This avoids Lean's bundled C compiler, which the
   Raylib.lean README reports as failing on Windows. Whether this holds on
   Windows is the open part of the Windows criterion. The proposal leaves
   "Lake or CMake" to this trial; so far CMake works on Linux.

7. **Test files** are downloaded by `tools/fetch_sst.py`, pinned to commit
   `f9c30210245d` of SingleStepTests/sm83 with a SHA-256 per file, and are not
   committed.

### Open

- Windows build and run (criterion 4), with the compiler to be used there.
- Choose the reference machine and repeat the benchmark on it.
- The binary links `libgcc_s` dynamically from the Lean toolchain directory
  on Linux; check what a release build should ship.
