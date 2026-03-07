# Basic Block Splitting (SPLIT)

Splits basic blocks at random points to multiply the number of blocks in each function. More blocks means more material for BCF (more candidates to clone), CFF (more switch cases in the dispatch loop), and SUB (more instructions to substitute). On its own, SPLIT does nothing to hinder analysis - its value is entirely as a force multiplier for subsequent passes.

Ported from Hikari's `SplitBasicBlocks.cpp` to the LLVM 17 new Pass Manager. Runs first in Phase 2 (before BCF, CFF, SUB), so every split block is visible to all three.

## How it works

Collects all basic blocks in the function, then processes each one independently. Blocks with fewer than 2 instructions, blocks with PHI nodes, exception handling pads, and blocks containing Swift error values are skipped.

For each eligible block, `splitNum` random split points are selected via Fisher-Yates shuffle. The selected indices are sorted so splits proceed front-to-back without invalidating earlier positions. Each split inserts an unconditional branch and moves subsequent instructions into a new block.

`splitNum` is clamped to block size minus 1 (can't produce more blocks than instructions) and hard-capped at 10 to prevent degenerate expansion.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_SPLIT` | off | Master switch |
| `SPLIT_NUM=n` | 2 | Splits per basic block (clamped to 1-10) |

## Per-function annotations

```c
// Enable for a specific function
__attribute__((annotate("split")))

// Disable for a specific function
__attribute__((annotate("nosplit")))

// Override split count
__attribute__((annotate("split split_num=4")))
```
