import Sm83.Step

/-!
# C interface

Functions exported with the C calling convention. The emulator owns a `Cpu`
object: `sm83_step` consumes it and returns the next one (updated in place
when the reference is unique). Exported functions cannot take borrowed
arguments, so the getters consume their argument too; `c/sm83.h` declares
them and wraps them in `sm83_peek_*` helpers that keep the caller's reference.
-/

namespace Sm83.Ffi

@[export sm83_new]
def new (a f b c d e h l : UInt8) (sp pc : UInt16) : Cpu :=
  Cpu.start { a, f, b, c, d, e, h, l, sp, pc }

@[export sm83_step]
def step (cpu : Cpu) (data irq : UInt8) : Cpu := cpu.step data irq

@[export sm83_request]
def request (cpu : Cpu) : UInt32 := cpu.request

/-- Registers at the last instruction boundary, by index:
0 A, 1 F, 2 B, 3 C, 4 D, 5 E, 6 H, 7 L, 8 SP, 9 PC, 10 locked. -/
@[export sm83_reg]
def reg (cpu : Cpu) (i : UInt8) : UInt16 :=
  let s := cpu.last
  match i with
  | 0 => s.a.toUInt16 | 1 => s.f.toUInt16 | 2 => s.b.toUInt16 | 3 => s.c.toUInt16
  | 4 => s.d.toUInt16 | 5 => s.e.toUInt16 | 6 => s.h.toUInt16 | 7 => s.l.toUInt16
  | 8 => s.sp | 9 => s.pc | 10 => if s.locked then 1 else 0
  | _ => 0

end Sm83.Ffi
