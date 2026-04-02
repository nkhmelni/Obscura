# Anti-Hook (AH)

I decided not to expose concrete internals of this pass since it's totally experimental and unique per this project. Though it's still exposed to any form of debugging of itself, I hope it will stay largely ambiguous for whoever tries to tamper with it in terms of the software it's applied to. Yet, I will proceed with a general documentation.

AH protects functions against two approaches to runtime tampering: code patching (function body modification, including inline hooking) and call redirection (GOT/stub rebinding, DYLD interposing, fishhook, just whatever stays invisible to call sites).

Works for AArch64 Darwin targets only (arm64e was briefly tested).

## How it works

The pass exposes two modes - `AH_INLINE` and `AH_REBIND`. Quite logically, the first one protects against any inline tampering (in bounds of a function), thereby restricting any `__text` modifications at the lowest level. This includes detection of any instruction bytes being patched, any inline hooking approaches (Dobby, Frida, etc.), and just whatever modifies functions in `__text`. The second one (`AH_REBIND`) protects all kinds of binds/rebases (external and internal indirect calls that go through GOT tables) from being captured by symbol-rebinding-based tampering approaches including interposing and fishhook. Additionally, when both modes are enabled, a "magical" coordination is achieved and a greater extent of tampering resistance is provided.

I suggest to use the full `AH_INLINE` + `AH_REBIND` mode with FCO and STRCRY (with full probability).

## Linker integration

`AH_REBIND` alone works out of the box — no extra build configuration.

`AH_INLINE` and combined mode need a post-link patching step that runs inside Apple's linker. The plugin ships `ld.sh` alongside `libObscura.dylib`. Pass it to clang as a linker flag (as shown in [Installation](../README.md#installation)).

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_AH` | off | Both inline + rebind |
| `ENABLE_AH_INLINE` | off | Code integrity only |
| `ENABLE_AH_REBIND` | off | Call protection only |
| `AH_PROB=n` | 100 | Unified probability for both (0-100) |
| `AH_INLINE_PROB=n` | 100 | Override for inline (0-100) |
| `AH_REBIND_PROB=n` | 100 | Override for rebind (0-100) |
| `AH_CALLBACK="name"` | none | Tamper detection callback (see below) |

## Per-function annotations

```c
OBSCURA_ANNOTATE("ah_inline")
OBSCURA_ANNOTATE("noah_inline")
OBSCURA_ANNOTATE("ah_inline ah_inline_prob=60")

OBSCURA_ANNOTATE("ah_rebind")
OBSCURA_ANNOTATE("noah_rebind")
OBSCURA_ANNOTATE("ah_rebind ah_rebind_prob=80")
```

## Tamper detection callback

`AH_CALLBACK` registers a function that AH calls before terminating the process when `AH_INLINE` detects code modification. Use it for logging, telemetry, or cleanup.

```c
// Signature must be void(const char*), function name is arbitrary
void ah_callback(const char *tampered_function_name);
```

- Define the function in exactly one translation unit (source file), whereas it must be a compiled TU (not a header).
- Set the AH_CALLBACK="ah_callback" flag (whatever you called your callback function), having `config.h` included in some way.
- Only fires for `AH_INLINE` detection (attributed to the design of all 3 modes). Coordinated mode (`AH_INLINE` + `AH_REBIND`) crashes implicitly.