/-!
# CPU state

Registers of the SM83 plus the extra CPU state the model needs. The flags live
in the upper nibble of `f` (Z = bit 7, N = bit 6, H = bit 5, C = bit 4); every
instruction keeps the lower nibble at zero (property P5).
-/

namespace Sm83

structure State where
  a  : UInt8
  f  : UInt8
  b  : UInt8
  c  : UInt8
  d  : UInt8
  e  : UInt8
  h  : UInt8
  l  : UInt8
  sp : UInt16
  pc : UInt16
  /-- Set by an invalid opcode: the CPU stops until power-off. In the
  prototype, opcodes not yet implemented behave the same way. -/
  locked : Bool := false
  deriving Repr, DecidableEq, Inhabited

/-- Little-endian 16-bit word from its two bytes. -/
@[inline] def word (hi lo : UInt8) : UInt16 := hi.toUInt16 <<< 8 ||| lo.toUInt16

@[inline] def hiByte (w : UInt16) : UInt8 := (w >>> 8).toUInt8
@[inline] def loByte (w : UInt16) : UInt8 := w.toUInt8

namespace State

@[inline] def bc (s : State) : UInt16 := word s.b s.c
@[inline] def hl (s : State) : UInt16 := word s.h s.l

@[inline] def zf (s : State) : Bool := s.f &&& 0x80 != 0
@[inline] def cf (s : State) : Bool := s.f &&& 0x10 != 0

end State

/-- The F register from the four flags. -/
@[inline] def mkFlags (z n h c : Bool) : UInt8 :=
  (if z then 0x80 else 0) ||| (if n then 0x40 else 0) |||
  (if h then 0x20 else 0) ||| (if c then 0x10 else 0)

end Sm83
