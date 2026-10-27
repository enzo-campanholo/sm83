/-!
# Bus programs

An instruction is a *bus program*: a sequence of reads, writes and cycles
without a transfer, one per M-cycle. `Bus` is the free monad over those three
operations, so instructions are written in direct style with `do` notation.

The same program has two interpretations:

* `Bus.run` (this file), a pure interpreter that executes the program against
  a memory and returns the result, the final memory and the list of bus events.
  The proofs and per-instruction reasoning use it.
* `Cpu.step` (`Sm83/Step.lean`), which suspends the program at every bus
  operation so the emulator can serve it, one M-cycle per call.
-/

namespace Sm83

inductive Bus (α : Type) where
  | pure  (a : α)
  | read  (addr : UInt16) (k : UInt8 → Bus α)
  | write (addr : UInt16) (val : UInt8) (k : Bus α)
  | idle  (k : Bus α)

namespace Bus

def bind : Bus α → (α → Bus β) → Bus β
  | .pure a,         f => f a
  | .read addr k,    f => .read addr (fun v => (k v).bind f)
  | .write addr v k, f => .write addr v (k.bind f)
  | .idle k,         f => .idle (k.bind f)

instance : Monad Bus where
  pure := .pure
  bind := bind

/-- What the bus did in one M-cycle. -/
inductive Event where
  | read  (addr : UInt16) (val : UInt8)
  | write (addr : UInt16) (val : UInt8)
  | idle
  deriving Repr, DecidableEq

abbrev Mem := UInt16 → UInt8

def Mem.set (m : Mem) (addr : UInt16) (v : UInt8) : Mem :=
  fun a => if a = addr then v else m a

/-- Pure interpreter: runs the program against `m`, answering each read from
the memory as it stands at that point. -/
def run : Bus α → Mem → α × Mem × List Event
  | .pure a, m => (a, m, [])
  | .read addr k, m =>
    let v := m addr
    let (a, m', es) := (k v).run m
    (a, m', .read addr v :: es)
  | .write addr v k, m =>
    let (a, m', es) := k.run (m.set addr v)
    (a, m', .write addr v :: es)
  | .idle k, m =>
    let (a, m', es) := k.run m
    (a, m', .idle :: es)

end Bus

end Sm83
