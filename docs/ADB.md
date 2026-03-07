# Anti-Debug (ADB)

Inserts inline ptrace-based debugger detection at function entry. Each protected function gets its own independent set of 3 `svc #0x80` syscalls with randomized register setup, tamper-detection logic emitted as normal LLVM IR, and a raw `SYS_exit` syscall on detection. No shared function, no precompiled bitcode - every insertion is unique and self-contained.

AArch64 Darwin only. Original to Obscura (not from Hikari).

Runs in Phase 1, before all code-generating and obfuscation passes. The tamper-check logic (icmp, and, condbr) is normal IR, so BCF, CFF, and SUB automatically obfuscate it in Phase 2.

## How it works

The entry block is split after stack variable declarations into an `entry` block and a `rest` block containing the original function body. Two new blocks are inserted between them: `check` and `exit`.

The `check` block contains 3 ptrace inline asm calls. Each call uses one of two **calling conventions**, selected randomly: direct ptrace (`x16=26`, `x0=31` for `PT_DENY_ATTACH`) or indirect syscall (`x16=0`, `x0=26` for `SYS_ptrace`, `x1=31`). The 5 register setup instructions within each call are **Fisher-Yates shuffled**, and each register randomly uses `x` (64-bit) or `w` (32-bit zero-extending) form. The kernel return value in `x0` is captured for the tamper check.

**Tamper detection**: if all 3 results are non-zero, the `svc` was NOP-patched. On normal execution, `ptrace(PT_DENY_ATTACH)` returns 0 on the first call. Subsequent calls may return `EPERM` (non-zero), but since the check is `r0 != 0 AND r1 != 0 AND r2 != 0`, a single successful call (result 0) makes the AND false - no false positive.

The `exit` block issues a raw `SYS_exit` syscall (`x16=1`) with a randomized exit code (1-200). No `_exit()` or `abort()` call - nothing hookable, no signal to catch.

## BCF interaction

BCF's `hasBCFRestriction` detects inline asm with register clobber constraints (`~{` in constraint string) and skips those blocks. ADB's ptrace blocks are not cloned into alteredBBs. The ADB check logic - normal IR icmp/and/condbr - is still obfuscated by CFF and SUB since those run after BCF.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_ADB` | off | Master switch (AArch64 Darwin only) |
| `ADB_PROB=n` | 40 | Per-function probability (0-100). Hash-based (`fnv1a(funcName, prngSeed)`) - deterministic and monotonic |

## Per-function annotations

```c
// Enable for a specific function
__attribute__((annotate("adb")))

// Disable for a specific function
__attribute__((annotate("noadb")))

// Override probability
__attribute__((annotate("adb adb_prob=80")))
```
