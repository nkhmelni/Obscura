# Constant Encryption (CONSTENC)

Encrypts global variable initializers at compile time and inserts decryption logic at each use site. The encrypted value is what gets baked into the binary's data section - a disassembler sees ciphertext, not the original constant. Supports integers (any bit width), floats (half/bfloat/float/double), arrays, and vectors.

This is the original Obscura encryption pass, not ported from Hikari. Hikari's `ConstantEncryption` operates at the instruction level (replacing constant operands like `add %x, 42` with XOR-decoded temporaries). CONSTENC operates at the global variable level - it covers float constants, arrays, vectors, and anything stored as a global initializer.

Strictly opt-in: requires `-DCONSTENC_LITE`, `-DCONSTENC_DEEP`, or `-DCONSTENC_FULL`. Without one of these, the pass is a no-op. Runs in Phase 1 (after L2G, before FCO and STRCRY). The decrypt code it inserts - XOR chains, salt loops, function calls - gets obfuscated by Phase 2 passes (BCF, CFF, SUB).

## How it works

Two encryption levels, independently or combined.

**Lite (L1)** encrypts the initializer with XOR and inserts inline XOR operations at every load and store. For integers, a single XOR with the derived key. For floats, the value is reinterpreted as an integer, XORed, and reinterpreted back. For arrays, the encrypted data is copied to a stack buffer at function entry and decrypted in place with a byte-level XOR loop. One XOR per load/store per iteration at default settings.

**Deep (L2)** encrypts the initializer with a 6-byte random salt and calls a decrypt function at each use site. The decrypt function (unique per module, randomized name) loops over the value's bytes, XORing each with the salt. `CONSTENC_DEEP_INLINE` inlines the decrypt function at every call site - eliminates call overhead, and each copy gets independently obfuscated by BCF/CFF/SUB.

When both are enabled (`CONSTENC_FULL`), Lite runs first, then Deep on top. Each level supports N iterations via `CONSTENC_LITE_TIMES`/`CONSTENC_DEEP_TIMES`. Deep iterations are forced odd internally - XOR is self-inverse, so even counts cancel out to plaintext.

Per-global key derivation uses a master key from the PRNG with per-iteration variation. With `PRNG_SEED` set, output is fully deterministic.

Arrays whose address escapes the function (returned, stored to a global, passed to a capturing call) are silently skipped. Globals that aren't exclusively used in loads and stores - dispatch_once tokens, ObjC runtime pointers, atomic variables - are also skipped. Only private/internal-linkage globals are eligible; compiler-generated constants are demoted to internal linkage first.

## STRCRY coordination

When STRCRY is active (`-DENABLE_STRCRY`), CONSTENC skips all integer-element arrays (`i8[]`, `i16[]`, `i32[]`, `i64[]`). STRCRY owns those - it encrypts them with per-string inline decrypt loops and mixed invertible operations. CONSTENC still handles float arrays, vectors, and all scalar types. Since CONSTENC runs before STRCRY in Phase 1, the globals are pristine when STRCRY processes them.

## L2G coordination

L2G-promoted globals get Lite encryption but skip Deep. Deep creates stack buffers at each use site; after CFF flattening, these end up inside the dispatch loop, and each iteration's buffer accumulates on the stack until function return - stack overflow on limited-stack threads. Variables marked `NO_CONSTENC` by the user are skipped entirely by CONSTENC.

## Flags

All flags require `#include "config.h"` and are passed as `-D` to the compiler.

| Flag | Default | Description |
|------|---------|-------------|
| `CONSTENC_LITE` | off | Enable Lite (L1) encryption |
| `CONSTENC_DEEP` | off | Enable Deep (L2) encryption |
| `CONSTENC_FULL` | off | Enable both Lite and Deep |
| `CONSTENC_PROB=n` | 50 | Per-global probability (0-100). Hash-based - deterministic and monotonic |
| `CONSTENC_LITE_TIMES=n` | 1 | Lite XOR iterations. Each adds one XOR per load/store |
| `CONSTENC_DEEP_TIMES=n` | 1 | Deep salt iterations. Forced odd (even cancels to plaintext) |
| `CONSTENC_FULL_TIMES=n` | - | Sets both LITE_TIMES and DEEP_TIMES |
| `CONSTENC_DEEP_INLINE` | off | Inline the decrypt function at all call sites |
| `CONSTENC_SKIP_NAME="p"` | - | Skip globals whose name contains any comma-separated pattern |
| `CONSTENC_SKIP_BITS="n"` | - | Skip globals of these bit widths (comma-separated) |
| `CONSTENC_SKIP_FLOATS` | off | Skip all float-type globals |
| `CONSTENC_SKIP_INTEGERS` | off | Skip all integer-type globals |
| `CONSTENC_ONLY_NAME="p"` | - | Only encrypt globals matching pattern |
| `CONSTENC_ONLY_BITS="n"` | - | Only encrypt globals of these bit widths |
| `CONSTENC_ONLY_FLOATS` | off | Only encrypt float-type globals |
| `CONSTENC_ONLY_INTEGERS` | off | Only encrypt integer-type globals |
| `CONSTENC_SKIP_ARRAYS` | off | Skip all array/vector encryption |
| `CONSTENC_ARRAYS_LITE_ONLY` | off | Arrays get Lite only, even with `CONSTENC_FULL` |

At least one of `CONSTENC_LITE`, `CONSTENC_DEEP`, or `CONSTENC_FULL` must be set. Without any, the pass does nothing.

## Per-variable annotations

```c
// Exclude a variable from encryption
NO_CONSTENC int skip_this = 42;

// Combined with L2G: promoted to global but not encrypted
L2G NO_CONSTENC int local_secret = 7;

// Per-function annotation (enable/disable for all constants in that function's scope)
OBSCURA_ANNOTATE("constenc")
OBSCURA_ANNOTATE("noconstenc")
OBSCURA_ANNOTATE("constenc_prob=80")
OBSCURA_ANNOTATE("constenc_times=3")
```
