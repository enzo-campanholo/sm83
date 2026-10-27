import Sm83
import Proofs.Spec
import Std.Tactic.BVDecide

/-!
# ADD A,B and ADD A,n8: result and flags

The trial's proof criterion (proposal, table 5) and the first two cases of P1.
For every state and every memory, running the instruction in the pure
interpreter yields the result and the flags of `Spec.Add8`, changes no other
register, writes nothing, and performs the documented bus cycles.
-/

namespace Sm83.Proofs

open Spec

/-- Unfolds an instruction's bus program until only data remains. `simp`
reduces the opcode `match` only with `decide := true` and the default simp set,
hence the separate step. -/
macro "unfold_instr" hop:term : tactic => `(tactic| (
  simp only [instr, instrM, imm8, Bus.run, $hop:term, rd, bind, pure, modify, modifyGet,
    MonadStateOf.modifyGet, get, getThe, MonadStateOf.get]
  simp (config := { decide := true }) only [exec]
  simp only [imm8, Bus.run, rd, bind, pure, modify, modifyGet, MonadStateOf.modifyGet, get,
    getThe, MonadStateOf.get]))

/-- The model's ADD satisfies the reference definition. -/
theorem add8_spec (a b : UInt8) : Add8 a b (add8 a b).1 (add8 a b).2 := by
  have hsum : ((a.toNat + b.toNat) % 256 == 0) = (a + b == 0) := by
    have h : (a + b).toNat = (a.toNat + b.toNat) % 256 := UInt8.toNat_add a b
    rw [← h, Bool.eq_iff_iff, beq_iff_eq, beq_iff_eq, ← UInt8.toNat_inj]
    rfl
  constructor
  · simp [add8, UInt8.toNat_add]
  · rw [hsum]
    simp only [flagZ, bit, add8, mkFlags]
    bv_decide
  all_goals
    simp only [flagN, flagH, flagC, bit, add8, mkFlags]
    bv_decide

theorem add_a_b (s : State) (m : Bus.Mem) (hop : m s.pc = 0x80) :
    let (s', m', events) := (instr s).run m
    Add8 s.a s.b s'.a s'.f ∧
    s' = { s with a := s'.a, f := s'.f, pc := s.pc + 1 } ∧
    m' = m ∧
    events = [.read s.pc 0x80] := by
  unfold_instr hop
  exact ⟨add8_spec s.a s.b, trivial, trivial, trivial⟩

theorem add_a_n8 (s : State) (m : Bus.Mem) (hop : m s.pc = 0xC6) :
    let n := m (s.pc + 1)
    let (s', m', events) := (instr s).run m
    Add8 s.a n s'.a s'.f ∧
    s' = { s with a := s'.a, f := s'.f, pc := s.pc + 2 } ∧
    m' = m ∧
    events = [.read s.pc 0xC6, .read (s.pc + 1) n] := by
  unfold_instr hop
  exact ⟨add8_spec _ _, by simp [UInt16.add_assoc], trivial, trivial⟩

end Sm83.Proofs
