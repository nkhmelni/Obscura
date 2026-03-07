# Indirect Branch Obfuscation (INDIBRAN)

Replaces direct branches with table-based indirect branches. Every `br label %target` becomes a GEP into a jump table, a load, and an `indirectbr`. Static CFG edges are severed - disassemblers can't resolve the target without tracing the table index computation, and the index is XOR-encrypted per-branch.

The core idea (global table for unconditional, local tables for conditional, shuffle blocks) is the same as in Hikari, but several things are added and optimized. It now handles arm64e PAC gracefully, skips BCF junk asm code, and fixes some more low-level bugs that aren't worth describing here.

Runs in Phase 3, after all code-generating passes (BCF, CFF, SUB, SPLIT, STRCRY, ADB).

## How it works

Two-pass algorithm over the module.

**Pass 1** walks all functions, checks eligibility (global flag, per-function annotation, probability), and filters out functions that have BCF junk asm blocks (read further to find out why).

**Pass 2** processes each eligible function: collects branch instructions to non-entry blocks, replaces unconditional branches with XOR-encrypted index lookups into a per-function global table, and replaces conditional branches with 2-entry local tables indexed by the zero-extended condition. Each `indirectbr` gets a shared unreachable decoy destination to prevent the backend from constant-folding it back to a direct branch. Finally, all non-entry blocks are shuffled (Fisher-Yates) to break layout correspondence with source order. Table pointers go through stack allocas so the table reference isn't directly visible as a global operand.

All generated globals use `PrivateLinkage` and randomized names. The linker strips them from the symbol table. They're registered in `llvm.compiler.used` to survive DCE.

## Tables

There are two table formats, selected automatically based on target:

**Relative offset tables** (arm64, x86_64). Each entry stores `blockaddr - tableaddr` as an i64 via `ConstantExpr::getSub`. These are link-time subtractor relocations, not runtime values. As a result, zero DATA-to-CODE rebase entries in `__DATA_CONST`. Disassemblers that scan rebase info to find basic block addresses (a common automated recovery technique) get nothing. At runtime, the offset is added back to the table base to recover the pointer - one ADD per branch.

**Pointer tables** (arm64e only). Each entry stores the raw `BlockAddress`. AppleClang's backend automatically PAC-signs each entry via the `ptrauth-indirect-gotos` function attribute and lowers `indirectbr` to `BRAA` (authenticated indirect branch). Every indirect branch is PAC-authenticated, and tampering with a table entry causes a hardware trap. Relative offset tables can't be used here because `ptrtoint` strips the pointer semantics that PAC operates on.

## BCF junk asm interaction

BCF_JUNKASM places `.long <random>` junk data in unreachable blocks behind opaque predicates. IDA's linear sweep hits the junk, loses instruction alignment, and can't recover the function. Very effective - IDA barely identifies functions.

If INDIBRAN converts any branch in that function to `indirectbr`, IDA stops at the first `br xN` in the entry block and never reaches the junk data. BCF's confusion is completely neutralized and IDA identifies all functions cleanly.

INDIBRAN handles this by skipping entire functions that have BCF junk asm blocks. Detection checks for the `"ov"` metadata tag (BCF's cross-pass marker) combined with an inline asm call. Regular BCF cloned blocks (no JUNKASM) have `"ov"` but no inline asm and are compatible with INDIBRAN.

In practice:

- `BCF(100%) + JUNKASM + INDIBRAN`: INDIBRAN skips all functions (or almost), BCF provides protection.
- `BCF(50%) + JUNKASM + INDIBRAN`: roughly half get BCF junk asm, half get INDIBRAN.
- `BCF(100%, no JUNKASM) + INDIBRAN`: both active on all functions, complementary, but the affect of junk asm isn't achieved.

To mix both across a codebase, use `BCF_PROB` to split the population, or use per-function annotations to assign each function to one approach.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_INDIBRAN` | off | Master switch |
| `INDIBRAN_PROB=n` | 100 | Per-function probability (0-100). Hash-based (`fnv1a(funcName, prngSeed)`) - deterministic and monotonic (raising the probability only adds functions, never removes) |

Index encryption and relative offset tables are always on. There's no flag to disable them - the overhead is negligible (1 XOR and 1 ADD per branch) and disabling either one leaves table indices or rebase entries trivially recoverable.

## Per-function annotations

```c
// Enable for a specific function
__attribute__((annotate("indibran")))

// Disable for a specific function
__attribute__((annotate("noindibran")))

// Override probability
__attribute__((annotate("indibran indibran_prob=50")))

// Hikari alias
__attribute__((annotate("indibr")))
```