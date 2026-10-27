import Lean
import Proofs

/-!
# Axiom check

Verification rule of the proposal (§5): each committed theorem may depend only
on `propext`, `Classical.choice` and `Quot.sound`, plus the axioms generated
for each use of `bv_decide`, whose names contain `._native.bv_decide.`. A gap
(`sorry`) would show up as `sorryAx` and fail the check.

Run with `lake env lean Proofs/CheckAxioms.lean`; it exits with an error if a
theorem breaks the rule.
-/

open Lean Elab Command

def allowedAxiom (n : Name) : Bool :=
  [``propext, ``Classical.choice, ``Quot.sound].contains n ||
    (n.toString.splitOn "._native.bv_decide.").length > 1

elab "#check_axioms " ids:ident* : command => do
  for id in ids do
    let name ← liftCoreM <| realizeGlobalConstNoOverloadWithInfo id
    let axioms ← liftCoreM <| collectAxioms name
    let bad := axioms.filter (!allowedAxiom ·)
    if bad.isEmpty then
      logInfo m!"{name}: {axioms.toList}"
    else
      logError m!"{name} depends on axioms outside the rule: {bad.toList}"

#check_axioms
  Sm83.Proofs.add8_spec
  Sm83.Proofs.add_a_b
  Sm83.Proofs.add_a_n8
