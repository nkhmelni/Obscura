# Bogus Control Flow (BCF)

Clones basic blocks behind opaque predicates that always evaluate true, creating unreachable **alteredBBs** that confuse disassemblers and decompilers. The cloned code looks plausible but never executes. With **junk asm** enabled, alteredBBs contain `.long <random>` data words that break instruction alignment - IDA's linear sweep loses synchronization and can't recover the function.

The core algorithm (clone block, opaque predicate, altered instructions) is from Hikari. Opaque predicate generation, junk asm modes, the cross-pass metadata tag for INDIBRAN coordination, and the inline-asm-clobber skip for ADB compatibility are new.

Runs in Phase 2 as a function pass, after all code-generating module passes (CONSTENC, FCO, STRCRY) and before CFF, SUB, and INDIBRAN.

## How it works

For each iteration (controlled by `BCF_LOOP`), BCF snapshots the function's eligible basic blocks and processes each one independently. Blocks added by previous iterations are picked up in subsequent ones - `BCF_LOOP=3` compounds the transformation.

Per-block probability is hash-based (`fnv1a` mixed with iteration and block index) - deterministic, monotonic, and independent of other passes' state.

**Block eligibility.** Blocks containing certain unsafe patterns are skipped: Swift error handling, coroutines, and inline asm with heavy register clobbers. The clobber check exists specifically for ADB's ptrace blocks - cloning them into alteredBBs creates downstream issues when combined with CFF+SUB+STRCRY+INDIBRAN. The ADB comparison/branch logic (normal IR) is still obfuscated by CFF and SUB since those run after BCF.

**Splitting and cloning.** Each eligible block is split. An **alteredBB** is created - either a full clone with mutated instructions (default) or an empty block (onlyJunkAsm mode). In the clone, integer and float operations get random junk instructions inserted, and comparison predicates are randomly swapped. None of this matters semantically since the block never executes, but it produces plausible-looking dead code in the decompiler.

**Opaque predicates.** Conditional branches are inserted routing to the original block (always-true path) or the alteredBB (never-taken path). Each predicate is backed by two global variables combined through a chain of random arithmetic operations. The chain length is `BCF_COND_COMPL` - more steps means more instructions an analyst must trace to prove the predicate is constant. The pass verifies at compile time that the predicate evaluates correctly, then sets the branch direction so the true path always reaches the original code.

**`BCF_CREATEFUNC`** moves the predicate computation into a separate function, called from the original function.

**Junk asm.** When `BCF_JUNKASM` is enabled, `.long <random>` inline asm directives are inserted into alteredBBs. The count is random between `BCF_JUNKASM_MINNUM` and `BCF_JUNKASM_MAXNUM`. These are raw 32-bit data words that disassemblers interpret as instructions, destroying alignment for the rest of the function. **`BCF_ONLYJUNKASM`** skips the clone entirely - the alteredBB contains only junk asm, producing a smaller footprint.

**INDIBRAN interaction.** Every instruction in an alteredBB is tagged with internal metadata. INDIBRAN reads this tag: if any block in a function has junk asm, INDIBRAN skips the entire function, because `indirectbr` at the entry block would prevent IDA from reaching the junk data that BCF placed deeper in the CFG. Regular BCF (no JUNKASM) is fully compatible with INDIBRAN.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_BCF` | off | Master switch |
| `BCF_PROB=n` | 70 | Per-block probability (0-100). Hash-based - deterministic and monotonic |
| `BCF_LOOP=n` | 1 | Iteration count (clamped 1-10). Each iteration processes all existing eligible blocks, including those created by prior iterations |
| `BCF_COND_COMPL=n` | 3 | Opaque predicate complexity (clamped 1-20). Number of chained arithmetic operations in each predicate |
| `BCF_JUNKASM` | off | Insert `.long <random>` data words into alteredBBs |
| `BCF_ONLYJUNKASM` | off | AlteredBBs contain only junk asm, no cloned code |
| `BCF_CREATEFUNC` | off | Move opaque predicate logic into separate functions |
| `BCF_JUNKASM_MINNUM=n` | 2 | Minimum junk data words per alteredBB |
| `BCF_JUNKASM_MAXNUM=n` | 4 | Maximum junk data words per alteredBB |

`BCF_JUNKASM` and `INDIBRAN` are incompatible at the per-function level. INDIBRAN automatically skips functions with junk asm blocks. Use `BCF_PROB` to split the population, or use per-function annotations to assign each function to one approach.

## Per-function annotations

```c
// Enable/disable BCF for this function
OBSCURA_ANNOTATE("bcf")
OBSCURA_ANNOTATE("nobcf")

// Override parameters
OBSCURA_ANNOTATE("bcf bcf_prob=100 bcf_loop=3 bcf_cond_compl=5")

// Toggle junk asm modes
OBSCURA_ANNOTATE("bcf bcf_junkasm")
OBSCURA_ANNOTATE("bcf bcf_onlyjunkasm")
OBSCURA_ANNOTATE("nobcf_junkasm")

// Junk asm count range
OBSCURA_ANNOTATE("bcf bcf_junkasm bcf_junkasm_minnum=4 bcf_junkasm_maxnum=8")

// Move predicates to separate functions
OBSCURA_ANNOTATE("bcf bcf_createfunc")
```
