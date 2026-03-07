# Function Wrapper (FUNCWRA)

Wraps call sites through intermediary functions. A call `foo(a, b)` becomes `wrapper(a, b) -> foo(a, b)`, with configurable nesting depth. Static call graph edges are severed - disassemblers and decompilers must resolve each wrapper to recover the original callee.

Ported from Hikari with fixes for symbol leakage, self-recursion, musttail, calling convention propagation, and tail call optimization. Hikari used `"FunctionWrapper"` as the function name with `InternalLinkage` - `nm -a` on the binary showed `_FunctionWrapper`, `_FunctionWrapper.1`, etc., a clear fingerprint. Obscura uses randomized `.fw.XXXXXX` names with `PrivateLinkage` (maps to `l_` prefix on macOS - stripped by the linker, zero trace in the binary).

Runs in Phase 3, after BCF, CFF, SUB, and SPLIT (Phase 2). This means wrapper bodies - a single call and return - are never obfuscated by those passes. Moving earlier wouldn't help much: the bodies have no arithmetic, branches, or loops for those passes to act on.

## How it works

Two-pass design. **Pass 1** walks every instruction in every eligible function, collects call sites that pass the probability check, and records them with their nesting depth. **Pass 2** creates the wrappers and rewrites the call sites. The split is necessary because creating functions during iteration invalidates the module's function list iterator.

Each wrapper is a one-basic-block function: forward arguments, call the callee, return the result. When there are no pass-by-value struct parameters, the inner call is marked as a tail call - on AArch64 this collapses the wrapper to a single `b` instruction (no frame setup). Calling convention is explicitly propagated from the original call site to the wrapper and its inner call. For varargs calls, each call site gets a wrapper with a fixed-argument type matching that site's actual arguments.

Skipped call sites: indirect calls (no resolved callee), compiler intrinsics and builtins, calls requiring strict tail-call semantics, functions with special parameter conventions (struct returns, Swift self), and direct self-recursive calls (wrapping `A -> A` with `FW_TIMES=2` would triple stack depth).

The probability check uses `fnv1a(funcName + callSiteIndex, prngSeed)` - deterministic and independent of other passes' PRNG consumption.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_FUNCWRA` | off | Master switch |
| `FW_PROB=n` | 30 | Per-call-site probability (0-100). Hash-based - deterministic and monotonic |
| `FW_TIMES=n` | 2 | Wrapper nesting depth. Each layer adds one intermediary function |

At `-O1` and above, LLVM's inliner may inline the trivial wrappers back into their callers, since the bodies are always below the inlining threshold. Wrappers reliably survive for `noinline` functions and external calls. The primary value is at `-O0`.

## Per-function annotations

```c
// Wrap every call in this function, 3 layers deep
__attribute__((annotate("funcwra funcwra_prob=100 funcwra_times=3")))

// Leave this function's calls alone
__attribute__((annotate("nofuncwra")))

// Hikari alias
__attribute__((annotate("fw")))
```
