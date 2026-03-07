# String Encryption (STRCRY)

Encrypts string constants at compile time and inserts per-string inline decryption loops at function entry. There is no shared decrypt function - each string gets its own small loop, so there is no single point to hook or breakpoint. Encrypted data lives in `__DATA`; on first call, an atomic-guarded path decrypts every string the function uses and flips a status flag. Subsequent calls skip straight to the function body.

The core idea (per-element encryption with inline decrypt loops) comes from Hikari. Significant additions: mixed invertible operations (XOR/ADD/SUB instead of XOR-only), randomized global names, hash-based per-element probability, multi-dimensional array support, ObjC CFString handling with cross-function dedup, and CONSTENC coordination.

Runs last in Phase 1 (after ADB, CONSTENC, FCO). Decrypt loops are normal IR - BCF, CFF, SUB, SPLIT, and INDIBRAN automatically obfuscate them in Phase 2+, making them indistinguishable from surrounding code.

## How it works

Single module pass, per-function processing.

For each eligible function, the pass collects all string references reachable from instructions. This includes strings nested inside struct and array initializers - the walk follows nested structures to arbitrary depth. ObjC `@"..."` literals are detected and their raw C strings extracted. Multi-dimensional arrays (`char table[][N]`) are flattened to 1D and encrypted as a single string - existing code accesses work transparently since the memory layout is identical.

Each string gets a random invertible operation (XOR, ADD, or SUB) and a random key array of the same length. Encryption is `enc[i] = op(plain[i], key[i])`. Three globals are created per string: encrypted data, a decrypt-space buffer (initialized to random garbage), and the key array. A fourth global holds the per-function status flag. All globals use randomized names with `PrivateLinkage` - the linker strips them entirely.

When the same raw string appears in multiple functions, they share a single encrypted/decrypt-space pair. Each function builds its own decrypt loop writing into the shared buffer - the writes are idempotent, so concurrent execution is safe.

At function entry, an atomic load of the status flag checks whether decryption has already run. If not, a chain of small loops executes - one per string - each doing `dst[i] = inverse_op(enc[i], key[i])`. After all loops, an atomic store sets the flag. Thread safety is lock-free: concurrent first calls all decrypt (idempotent), then all set the flag.

ObjC CFString wrappers (`@"..."` literals) are patched at module level after all per-function processing. Each wrapper is rewritten to point to the decrypted buffer instead of the original plaintext. Dead plaintext globals are erased.

## CONSTENC coordination

STRCRY owns all integer-element arrays (i8/i16/i32/i64). CONSTENC owns scalars, floats, float arrays, and vectors. CONSTENC runs before STRCRY in Phase 1, so STRCRY's generated globals don't exist yet - no coordination needed. When STRCRY is enabled, CONSTENC automatically excludes integer-element arrays from its processing.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_STRCRY` | off | Master switch |
| `STRCRY_PROB=n` | 100 | Per-element encryption probability (0-100). Hash-based - deterministic and monotonic (raising the probability only adds encrypted elements, never removes) |

Hikari's `ENABLE_STRENC` alias works too.

## Per-function annotations

```c
// Enable for a specific function
__attribute__((annotate("strcry")))

// Disable for a specific function
__attribute__((annotate("nostrcry")))

// Override probability
__attribute__((annotate("strcry strcry_prob=50")))

// Hikari alias
__attribute__((annotate("strenc")))
```
