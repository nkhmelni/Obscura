# Local-to-Global Promotion (L2G)

Promotes local constant variables to global variables so that CONSTENC can encrypt them. CONSTENC operates on globals - it can't see function-local constants that live on the stack. L2G bridges the gap by moving those values into the data section before CONSTENC runs. Original to Obscura (not from Hikari).

Runs in Phase 1, immediately before CONSTENC. Generated globals are tagged with metadata so CONSTENC can identify them.

## How it works

Two modes: **marker mode** and **automatic mode**.

Marker mode is always active. The user annotates individual variables with the `L2G` macro (`OBSCURA_ANNOTATE("l2g")`), and L2G promotes exactly those variables. No `-D` flag needed - if any `l2g` annotation exists in the module, the pass processes it.

Automatic mode is enabled by `-DL2G_ENABLE`. All eligible constants in all functions are promoted, subject to type flags and probability. The two modes compose - marker promotions run first, then automatic promotions (arrays first, then scalar operands).

Promotion handles two patterns. Scalar constants stored to a local variable are replaced with a global carrying the same value. Arrays initialized by copying from a compiler-generated constant are replaced with a new global carrying the same initializer. In both cases, the local variable is eliminated.

Before promoting in automatic mode, the pass checks that nothing writes to the variable after initialization. A global retains modified values across function calls - promoting a mutable local would silently corrupt subsequent invocations. Marker-mode promotions skip this check (the user explicitly requested it).

Scalar arguments to external and indirect function calls are skipped in automatic mode. Only arguments to locally-defined functions are promoted. This prevents subtle runtime failures with ObjC selector stubs and system framework functions where the additional indirection causes ABI issues.

All generated globals use `PrivateLinkage` and randomized names. When deduplication is enabled (`-DL2G_DEDUP`), identical constant values share the same global - but marker-mode promotions never deduplicate, since each annotated variable is mutable storage and aliasing them would be incorrect.

When STRCRY is active in the same build, automatic mode skips integer-element arrays and vectors. STRCRY owns these types - promoting them to globals would cause a wasteful round-trip, since STRCRY creates local decrypt copies anyway. Marker-mode promotions ignore this exclusion.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `L2G_ENABLE` | off | Enable automatic promotion of all eligible constants |
| `L2G_PROB=n` | 100 | Per-decision probability (0-100). Hash-based - deterministic and monotonic |
| `L2G_INTEGERS=0/1` | 1 | Promote integer scalar operands |
| `L2G_FLOATS=0/1` | 1 | Promote floating-point scalar operands |
| `L2G_INT_ARRAYS=0/1` | 1 | Promote integer arrays and vectors |
| `L2G_FLOAT_ARRAYS=0/1` | 1 | Promote floating-point arrays and vectors |
| `L2G_OPS` | off | Promote binary operation results (store result to global, load back) |
| `L2G_DEDUP` | off | Deduplicate identical constants (automatic mode only) |
| `L2G_MAX_ARRAY=n` | 1024 | Maximum array element count to promote (0 = unlimited) |

Type flags default to all-on when `L2G_ENABLE` is set. Use `L2G_INTEGERS=0` etc. to disable specific types. Without `L2G_ENABLE`, only marker-annotated variables are processed (type flags are ignored for markers).

## Per-variable annotations

L2G uses per-variable annotations, not per-function. These work without any `-D` flags.

```c
#include "config.h"

// Promote this variable to a global (CONSTENC will encrypt it)
L2G int secret_key = 0xDEADBEEF;

// Promote but exclude from CONSTENC encryption
L2G NO_CONSTENC int promoted_but_unencrypted = 42;

// Exclude from automatic promotion (only relevant with -DL2G_ENABLE)
NO_L2G int keep_local = 123;
```

`L2G` and `NO_CONSTENC` can appear in either order. `NO_CONSTENC` prevents CONSTENC from encrypting the promoted global.
