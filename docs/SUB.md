# Instruction Substitution (SUB)

Replaces integer binary operators with semantically equivalent but more complex instruction sequences. A single `add` becomes 3-7 instructions; a single `xor` becomes 5-14. The original opcode disappears from the IR entirely, forcing an analyst to recognize algebraic identities to recover the computation. Stacks well with multiple iterations - each pass substitutes the instructions introduced by the previous one, compounding the complexity exponentially.

Ported from Hikari's `Substitution.cpp` + `SubstituteImpl.cpp`. The substitution patterns are unchanged; the driver is rewritten for the LLVM 17 new Pass Manager with hash-based probability replacing the shared PRNG roll. Runs in Phase 2 alongside BCF, CFF, and SPLIT - after all code-generating module passes, so decrypt stubs from STRCRY and CONSTENC are substituted too.

## How it works

Iterates all instructions in the function `SUB_LOOP` times. On each iteration, every binary operation is checked: if it's one of the 6 handled opcodes and passes the probability gate, it's replaced in-place. The pass randomly selects one pattern from that opcode's pool and builds the replacement sequence, removing the original instruction.

**33 patterns** across 6 opcodes: ADD (7), SUB (6), AND (6), OR (6), XOR (6), MUL (2). Patterns range from simple algebraic rewrites (`a + b => a - (-b)`) to bitwise decompositions (`a + b => (a ^ b) + 2*(a & b)`) to randomized identity insertions (`a + b => a + r + b - r` where `r` is a random constant). AND and OR each have NOR-based and NAND-based variants that internally randomize between two equivalent NOR/NAND constructions (`~(a | b)` vs `~a & ~b`), adding further variation.

The probability decision uses `fnv1a` hashing, deterministic for a given (function, seed, iteration, instruction index) tuple and independent of other passes' state - changing `SUB_PROB` from 50 to 80 only adds instructions, never removes ones that were previously substituted.

With `SUB_LOOP > 1`, the substituted instructions from iteration N become candidates in iteration N+1. A single `add` at `SUB_LOOP=3` can expand to dozens of instructions. Loop count is clamped to [1, 10].

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_SUB` | off | Master switch |
| `SUB_PROB=n` | 50 | Per-instruction probability (0-100). Hash-based - deterministic and monotonic |
| `SUB_LOOP=n` | 1 | Iteration count. Each iteration re-substitutes the output of the previous one. Clamped to [1, 10] |

`SUB_PROB=100` substitutes every eligible instruction. At the default 50%, roughly half are replaced per iteration - enough to obscure arithmetic without excessive code growth. `SUB_LOOP=2` or `3` is the practical sweet spot for aggressive protection; beyond that, diminishing returns against binary size.

The `ENABLE_SUBOBF` spelling is accepted as a Hikari compatibility alias for `ENABLE_SUB`.

## Per-function annotations

```c
// Enable for this function only
__attribute__((annotate("sub")))

// Disable for this function
__attribute__((annotate("nosub")))

// Override probability and loop count
__attribute__((annotate("sub sub_prob=100 sub_loop=3")))
```
