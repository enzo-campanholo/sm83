import Sm83.Exec

/-!
# Step-by-step execution

The second interpretation of bus programs. `Cpu` holds the rest of the current
instruction's program; its head is the request for the current M-cycle. The
emulator serves the request (reading or writing memory, advancing the other
components four clocks) and calls `Cpu.step` with the data read, which advances
exactly one M-cycle.
-/

namespace Sm83

structure Cpu where
  /-- The rest of the current instruction. `.pure` only when the CPU is locked. -/
  prog : Bus State
  /-- State at the last instruction boundary. -/
  last : State
  /-- The current request is the opcode fetch of a new instruction, so the
  previous instruction ended in the last M-cycle. -/
  boundary : Bool

def Cpu.start (s : State) : Cpu :=
  { prog := if s.locked then .pure s else instr s, last := s, boundary := true }

/-- Continues with `p`, the program after the current M-cycle. When the
instruction is over, the next one starts. -/
@[inline] def Cpu.advance (c : Cpu) : Bus State → Cpu
  | .pure s => Cpu.start s
  | p       => { c with prog := p, boundary := false }

/-- Advances one M-cycle. `data` answers the current request when it is a read
and is ignored otherwise. `irq` (pending interrupts) is part of the contract
but unused until interrupts are modelled. -/
def Cpu.step (c : Cpu) (data : UInt8) (_irq : UInt8) : Cpu :=
  match c.prog with
  | .pure _      => { c with boundary := false }
  | .read _ k    => c.advance (k data)
  | .write _ _ k => c.advance k
  | .idle k      => c.advance k

/-! ## Request encoding for C

`Cpu.request` packs the current request in a `UInt32`:

| bits  | field                                         |
|-------|-----------------------------------------------|
| 0–15  | address (read, write)                         |
| 16–23 | value (write)                                 |
| 24–25 | kind: 0 read, 1 write, 2 idle, 3 stopped      |
| 26    | boundary: this is the fetch of a new instruction |
-/

def Cpu.request (c : Cpu) : UInt32 :=
  let r : UInt32 := match c.prog with
    | .read a _    => a.toUInt32
    | .write a v _ => (0x0100_0000 : UInt32) ||| (v.toUInt32 <<< 16) ||| a.toUInt32
    | .idle _      => 0x0200_0000
    | .pure _      => 0x0300_0000
  if c.boundary then r ||| 0x0400_0000 else r

end Sm83
