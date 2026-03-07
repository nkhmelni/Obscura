# Control Flow Flattening (CFF)

Replaces structured control flow with a switch-dispatch loop. Every branch becomes a store to a dispatch variable followed by a jump back to the loop header, which loads the variable, hits a switch, and dispatches to the next block. The original CFG edges disappear - disassemblers and decompilers see a single flat switch over opaque integer values instead of nested if/else or loop structures.

Ported from Hikari with several fixes: collision-free case value scrambling, support for `noreturn` functions, and shuffled dispatch order. Runs in Phase 2 (function-level obfuscation), after SplitBasicBlocks.

## How it works

First, any existing switch statements in the function are lowered to if-else chains - CFF needs to be the only switch in the function.

The pass collects all basic blocks with simple terminators (branches, returns, unreachable). If any block uses exception handling or has a complex terminator, the function is skipped entirely. Functions with one or zero eligible blocks are also skipped.

The entry block is separated from the dispatch list - it becomes the preamble that initializes the dispatch variable and falls through to the loop header. If the entry block ends with a conditional branch, it is split so the condition evaluation enters the dispatch as a normal block.

The entry block's actual successor is placed at position 0 in the block list, because the dispatch variable's initial value maps to that position. The remaining blocks are shuffled with Fisher-Yates to prevent the switch case ordering from leaking the original block layout.

**Scrambling** maps each block index to a unique random i32. On collision (same random value already assigned to a different index), a new value is generated. This eliminates duplicate case values that Hikari's original scrambling could produce with large functions.

The dispatch loop is three blocks: loop entry (load dispatch variable, execute switch), switch default (fallthrough for unmatched values), and loop end (the back-edge target). Each original block is wired into the switch as a case:

- **0 successors** (return, unreachable): left as-is.
- **1 successor**: the branch is replaced with a store of the successor's case value, then a jump to loop end.
- **2 successors** (conditional branch): a select picks the true or false successor's case value based on the original condition, stores it, and jumps to loop end.

After rewiring, all cross-block value dependencies are demoted to stack storage. The dispatch loop breaks the normal dominance structure (every block is reachable only through the switch), so values that previously flowed between blocks must go through memory.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_CFF` | off | Master switch |
| `CFF_PROB=n` | 100 | Per-function probability (0-100). Hash-based (`fnv1a(funcName, prngSeed)`) - deterministic and monotonic |

Aliases: `ENABLE_FLA` and `ENABLE_CFFOBF` both resolve to `ENABLE_CFF`.

CFF has no iteration count flag - one pass is sufficient. Unlike BCF (which layers cloned blocks) or SUB (which re-substitutes), running CFF twice would just re-flatten an already-flat switch, producing no additional protection.

## Per-function annotations

```c
// Enable for a specific function
__attribute__((annotate("cff")))

// Disable for a specific function
__attribute__((annotate("nocff")))

// Override probability
__attribute__((annotate("cff cff_prob=50")))

// Hikari aliases
__attribute__((annotate("fla")))       // -> cff
__attribute__((annotate("cffobf")))    // -> cff
```
