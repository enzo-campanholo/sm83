import Lake
open Lake DSL

package sm83

/-- The executable model of the CPU. It imports only `Init`, so a C program
links against `libInit` and the Lean runtime and nothing else. -/
@[default_target]
lean_lib Sm83 where
  defaultFacets := #[LeanLib.staticFacet]

/-- Proofs about the model. Never linked into the emulator. -/
@[default_target]
lean_lib Proofs
