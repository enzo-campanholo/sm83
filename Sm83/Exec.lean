import Sm83.State
import Sm83.Bus

/-!
# Instructions

Each instruction is a bus program in `M`, a state monad over `Bus`. The opcode
fetch is the first M-cycle of every instruction (the convention of the
SingleStepTests and of `Opcodes.json`); on hardware it overlaps the last
M-cycle of the previous instruction, which is invisible at M-cycle precision.

Prototype scope: the thirteen opcodes below. Every other opcode locks the CPU.
-/

namespace Sm83

/-- State and bus, in continuation-passing style: a computation receives the
state and the rest of the instruction. Equivalent to `StateT State Bus`, but
a bind composes functions instead of rebuilding the bus program, which made
the compiled model about four times as fast in the trial (`DECISIONS.md`). -/
def M (α : Type) := State → (α → State → Bus State) → Bus State

instance : Monad M where
  pure a := fun s k => k a s
  bind x f := fun s k => x s (fun a s' => f a s' k)

instance : MonadStateOf State M where
  get := fun s k => k s s
  set s := fun _ k => k () s
  modifyGet f := fun s k => let (a, s') := f s; k a s'

/-! ## Bus operations, one M-cycle each -/

@[inline] def rd (addr : UInt16) : M UInt8 := fun s k => .read addr (fun v => k v s)
@[inline] def wr (addr : UInt16) (v : UInt8) : M Unit := fun s k => .write addr v (k () s)
/-- An M-cycle without a bus transfer. -/
@[inline] def internal : M Unit := fun s k => .idle (k () s)

/-! ## Operand access -/

@[inline] def imm8 : M UInt8 := do
  let v ← rd (← get).pc
  modify fun s => { s with pc := s.pc + 1 }
  pure v

@[inline] def imm16 : M UInt16 := do
  let lo ← imm8
  let hi ← imm8
  pure (word hi lo)

/-- Writes the high byte to SP − 1, then the low byte to SP − 2 (P7). -/
@[inline] def push16 (w : UInt16) : M Unit := do
  modify fun s => { s with sp := s.sp - 1 }
  wr (← get).sp (hiByte w)
  modify fun s => { s with sp := s.sp - 1 }
  wr (← get).sp (loByte w)

@[inline] def pop16 : M UInt16 := do
  let lo ← rd (← get).sp
  modify fun s => { s with sp := s.sp + 1 }
  let hi ← rd (← get).sp
  modify fun s => { s with sp := s.sp + 1 }
  pure (word hi lo)

/-- Sign extension of the `e8` operand of relative jumps. -/
@[inline] def sext (e : UInt8) : UInt16 := e.toInt8.toInt16.toUInt16

/-! ## ALU -/

/-- ADD: result and new F. -/
@[inline] def add8 (a b : UInt8) : UInt8 × UInt8 :=
  let r := a + b
  (r, mkFlags (r == 0) false ((a &&& 0xF) + (b &&& 0xF) > 0xF) (r < a))

/-- INC: result and new F; C is kept. -/
@[inline] def inc8 (v f : UInt8) : UInt8 × UInt8 :=
  let r := v + 1
  (r, mkFlags (r == 0) false (v &&& 0xF == 0xF) (f &&& 0x10 != 0))

/-! ## Decode and execute -/

/-- The body of the instruction with opcode `op`, after the fetch. -/
def exec (op : UInt8) : M Unit :=
  match op with
  | 0x00 => pure ()                                     -- NOP
  | 0x04 => modify fun s =>                             -- INC B
      let (r, f) := inc8 s.b s.f
      { s with b := r, f := f }
  | 0x06 => do                                          -- LD B,n8
      let n ← imm8
      modify fun s => { s with b := n }
  | 0x20 => do                                          -- JR NZ,e8
      let e ← imm8
      unless (← get).zf do
        internal
        modify fun s => { s with pc := s.pc + sext e }
  | 0x77 => do                                          -- LD (HL),A
      let s ← get
      wr s.hl s.a
  | 0x7E => do                                          -- LD A,(HL)
      let v ← rd (← get).hl
      modify fun s => { s with a := v }
  | 0x80 => modify fun s =>                             -- ADD A,B
      let (r, f) := add8 s.a s.b
      { s with a := r, f := f }
  | 0xC3 => do                                          -- JP a16
      let nn ← imm16
      internal
      modify fun s => { s with pc := nn }
  | 0xC5 => do                                          -- PUSH BC
      internal
      push16 (← get).bc
  | 0xC6 => do                                          -- ADD A,n8
      let n ← imm8
      modify fun s =>
        let (r, f) := add8 s.a n
        { s with a := r, f := f }
  | 0xC9 => do                                          -- RET
      let nn ← pop16
      internal
      modify fun s => { s with pc := nn }
  | 0xCD => do                                          -- CALL a16
      let nn ← imm16
      internal
      push16 (← get).pc
      modify fun s => { s with pc := nn }
  | 0xF1 => do                                          -- POP AF
      let w ← pop16
      modify fun s => { s with a := hiByte w, f := loByte w &&& 0xF0 }
  | _ => modify fun s => { s with locked := true }

/-- One whole instruction: fetch, then execute. -/
def instrM : M Unit := do
  let op ← imm8
  exec op

/-- The bus program of the instruction at `s.pc`, returning the state after it. -/
def instr (s : State) : Bus State := instrM s (fun _ s' => .pure s')

end Sm83
