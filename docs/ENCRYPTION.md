# Encryption

This document covers the encryption levels, options, and behavior of the Encryption Pass.

## Overview

The Encryption Pass protects global variable initializers by encrypting their values at compile time and inserting runtime decryption code. This makes static analysis significantly harder, as sensitive data is not visible in the binary's data sections.

> **Important:** Compile with `-O0` or `-O1` for proper obfuscation. Higher optimization levels may inline or eliminate the decryption code, causing incorrect decryption results and wrong final values.

## Encryption Levels

Three encryption levels are available:

| Level | Flag | Description |
|-------|------|-------------|
| Lite | `ENC_LITE` | Lightweight XOR-based encryption with inline decryption |
| Deep | `ENC_DEEP` | Salt-based encryption with function call decryption |
| Full | `ENC_FULL` | Both Lite and Deep combined (maximum protection) |

### Lite Encryption

Lite encryption uses a fast XOR-based algorithm. Decryption code is always inlined at each usage site, making it efficient but providing basic protection.

### Deep Encryption

Deep encryption uses a more complex salt-based algorithm. By default, decryption is performed via a function call, but you can inline it for additional obfuscation (see [Inline Mode](#inline-mode)).

### Full Encryption

Full encryption applies both Lite and Deep levels sequentially. This is equivalent to specifying both `ENC_LITE` and `ENC_DEEP`.

## Default Behavior

The pass operates in two modes depending on whether you include `config.h`:

| Configuration | Behavior |
|---------------|----------|
| **No header included** | Full encryption runs automatically (times=1, no inlining) |
| **Header included, no flags** | No encryption (explicit opt-in required) |
| **Header included with flags** | Only the specified encryption level runs |

When you don't include the header, the pass assumes you want maximum protection with sensible defaults. This is useful for quick protection without configuration.

When you include the header, you gain explicit control. Nothing runs unless you specify what you want.

## Usage

### Basic Usage (Implicit Mode)

```bash
# Full encryption with defaults (no configuration needed)
clang -fpass-plugin=path/to/libObscura.dylib program.c
```

### Explicit Mode

```bash
# Include the header for explicit control
clang -fpass-plugin=path/to/libObscura.dylib \
      -DENC_FULL \
      -I path/to/include -include config.h \
      program.c
```

### Individual Levels

```bash
# Lite encryption only
clang ... -DENC_LITE -I path/to/include -include config.h program.c

# Deep encryption only
clang ... -DENC_DEEP -I path/to/include -include config.h program.c

# Full encryption (both levels)
clang ... -DENC_FULL -I path/to/include -include config.h program.c
```

## Encryption Options

### Iteration Count

You can apply encryption multiple times for increased obfuscation:

| Flag | Description |
|------|-------------|
| `ENC_LITE_TIMES=n` | Apply Lite encryption `n` times |
| `ENC_DEEP_TIMES=n` | Apply Deep encryption `n` times |
| `ENC_FULL_TIMES=n` | Set both iteration counts to `n` |

All values are supported, but going beyond 15 is not recommended for performance reasons.

```bash
# 3 iterations of Lite encryption
clang ... -DENC_LITE -DENC_LITE_TIMES=3 ...

# 5 iterations of both levels
clang ... -DENC_FULL -DENC_FULL_TIMES=5 ...
```

### Inline Mode

| Flag | Description |
|------|-------------|
| `ENC_DEEP_INLINE` | Inline Deep decryption at each usage site |

By default, Deep encryption uses a decryption function that gets called at runtime. With `ENC_DEEP_INLINE`, the decryption logic is inlined at every place the variable is used, scattering the code throughout the binary.

```bash
# Deep encryption with inlined decryption
clang ... -DENC_DEEP -DENC_DEEP_INLINE ...

# Full encryption with inlined Deep decryption
clang ... -DENC_FULL -DENC_DEEP_INLINE ...
```

> **Warning**
>
> Using a high iteration count (~10 or more) together with `ENC_DEEP_INLINE` can cause some disassemblers to fail when generating pseudocode. For example, IDA Pro may display an error instead of decompiled code.
>
> <!-- TODO: Add screenshot showing IDA error -->
>
> This is actually a side effect that increases protection, but be aware of it during development and debugging.

> **Note**
>
> The `ENC_DEEP_INLINE_PROB` option (probabilistic inlining) is planned for a future release.

## Supported Types

The encryption pass handles:

- **Integers**: Any bit width (8, 16, 32, 64, etc.), including `char`
- **Floats**: half, bfloat, float, double
- **Arrays**: Integer and float arrays
- **Vectors**: SIMD vector types

Since `char` is an integer type and C strings are character arrays, primitive string literals stored in global variables are also encrypted.

## Example

```c
#include <stdio.h>
#include <stdint.h>

static int32_t secret_key = 0xDEADBEEF;
static float magic_value = 3.14159f;
static int32_t lookup[4] = {10, 20, 30, 40};

int main(void) {
    printf("Key: 0x%08X\n", secret_key);
    printf("Magic: %f\n", magic_value);
    printf("Sum: %d\n", lookup[0] + lookup[1]);
    return 0;
}
```

Compile with encryption:

```bash
clang -fpass-plugin=path/to/libObscura.dylib \
      -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE \
      -I path/to/include -include config.h \
      example.c -o example
```

The program runs identically, but static analysis tools won't see the original values in the binary.

## See Also

- [Filters](FILTERS.md) - Control which variables get encrypted
- [Local-to-Global Promotion](L2G.md) - Promote local constants for encryption
- [Combined Usage](COMBINED.md) - Using all features together
