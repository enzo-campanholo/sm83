/-!
# Reference definitions

Arithmetic written apart from the model (independence rule, proposal §5): the
statements of the theorems use only these definitions and the semantics of the
model (`Bus.run` and `Cpu.step`), never the model's helper functions.

Sums that may exceed a byte are computed in natural numbers or in 16 bits,
where they cannot overflow.
-/

namespace Spec

/-- Bit `i` of `x`. -/
def bit (x : UInt8) (i : UInt8) : Bool := (x >>> i) % 2 == 1

/-- The flags, by position in F. -/
def flagZ (f : UInt8) : Bool := bit f 7
def flagN (f : UInt8) : Bool := bit f 6
def flagH (f : UInt8) : Bool := bit f 5
def flagC (f : UInt8) : Bool := bit f 4

/-- `r` and `f` are the result and the flags of the 8-bit addition `a + b`:
the sum modulo 256; Z if that sum is zero; N clear; H if the lower nibbles
carry (their sum is at least 16); C if the full sum is at least 256; the lower
nibble of F clear. -/
structure Add8 (a b r f : UInt8) : Prop where
  result : r.toNat = (a.toNat + b.toNat) % 256
  zero   : flagZ f = ((a.toNat + b.toNat) % 256 == 0)
  sub    : flagN f = false
  half   : flagH f = (a % 16 + b % 16 ≥ 16)
  carry  : flagC f = (a.toUInt16 + b.toUInt16 ≥ 256)
  low    : f % 16 = 0

end Spec
