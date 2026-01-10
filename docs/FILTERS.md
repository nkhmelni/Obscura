# Filters

Filters give you fine-grained control over which variables are encrypted. You can exclude specific variables, target only certain types, or filter by name patterns.

## Filter Types

There are two categories of filters:

| Category | Prefix | Behavior |
|----------|--------|----------|
| **Blacklist** | `ENC_SKIP_*` | Exclude matching variables from encryption |
| **Whitelist** | `ENC_ONLY_*` | Encrypt only matching variables |

When both are used, whitelist filters are applied first, then blacklist exclusions.

## Per-Variable Annotation

The `NO_ENC` annotation excludes a specific variable from encryption. This works on both global and local variables.

```c
#include "config.h"

NO_ENC static int32_t public_counter = 0;  // Never encrypted
static int32_t secret_key = 0x12345678;    // Encrypted normally
```

`NO_ENC` takes precedence over all other filters. Even if a variable matches whitelist criteria, `NO_ENC` will exclude it.

### Combining with L2G

You can combine `NO_ENC` with the `L2G` annotation in any order:

```c
L2G NO_ENC int32_t val = 42;  // Promoted to global, but NOT encrypted
NO_ENC L2G int32_t val = 42;  // Same effect - order doesn't matter
```

This is useful when you want a local constant promoted for obfuscation purposes but don't want it encrypted (perhaps for performance reasons or debugging).

## Name Pattern Filters

Filter variables by substring matching on their names:

| Flag | Description |
|------|-------------|
| `ENC_SKIP_NAME="pattern"` | Skip variables containing pattern |
| `ENC_ONLY_NAME="pattern"` | Encrypt only variables containing pattern |

Multiple patterns can be specified with commas:

```bash
# Skip variables with "public" or "debug" in their names
clang ... -DENC_FULL '-DENC_SKIP_NAME="public,debug"' ...

# Encrypt only variables with "secret" or "key" in their names
clang ... -DENC_FULL '-DENC_ONLY_NAME="secret,key"' ...
```

> **Note**
>
> Pattern matching is case-sensitive and checks if the pattern appears anywhere in the variable name.

## Bit Size Filters

Filter variables by their bit width:

| Flag | Description |
|------|-------------|
| `ENC_SKIP_BITS="n"` | Skip variables with specified bit sizes |
| `ENC_ONLY_BITS="n"` | Encrypt only variables with specified bit sizes |

Multiple sizes can be specified:

```bash
# Skip 8-bit and 16-bit variables
clang ... -DENC_FULL '-DENC_SKIP_BITS="8,16"' ...

# Encrypt only 32-bit and 64-bit variables
clang ... -DENC_FULL '-DENC_ONLY_BITS="32,64"' ...
```

## Type Filters

Filter by data type category:

| Flag | Description |
|------|-------------|
| `ENC_SKIP_FLOATS` | Skip all floating-point types |
| `ENC_SKIP_INTEGERS` | Skip all integer types |
| `ENC_ONLY_FLOATS` | Encrypt only floating-point types |
| `ENC_ONLY_INTEGERS` | Encrypt only integer types |

```bash
# Encrypt only integers, skip all floats
clang ... -DENC_FULL -DENC_ONLY_INTEGERS ...

# Skip floating-point variables
clang ... -DENC_FULL -DENC_SKIP_FLOATS ...
```

## Array Filters

Special controls for array encryption:

| Flag | Description |
|------|-------------|
| `ENC_SKIP_ARRAYS` | Skip all array encryption |
| `ENC_ARRAYS_LITE_ONLY` | Arrays receive only Lite encryption |

Arrays can be expensive to encrypt, especially large ones. These options let you balance protection with performance:

```bash
# Skip array encryption entirely
clang ... -DENC_FULL -DENC_SKIP_ARRAYS ...

# Full encryption for scalars, Lite only for arrays
clang ... -DENC_FULL -DENC_ARRAYS_LITE_ONLY ...
```

## Filter Precedence

Filters are applied in this order:

1. **Whitelist** (`ENC_ONLY_*`) - If any whitelist is active, only matching variables are considered
2. **Blacklist** (`ENC_SKIP_*`) - Matching variables are excluded
3. **Annotation** (`NO_ENC`) - Always excludes the variable

This means:
- A variable must pass the whitelist (if active)
- Then it must not match any blacklist
- Then it must not have `NO_ENC`

## Combining Filters

Filters can be combined for precise control:

```bash
# Encrypt only 32-bit integers, skip arrays
clang ... -DENC_FULL \
      -DENC_ONLY_INTEGERS \
      '-DENC_ONLY_BITS="32"' \
      -DENC_SKIP_ARRAYS \
      ...

# Skip public/debug variables and all floats
clang ... -DENC_FULL \
      '-DENC_SKIP_NAME="public,debug"' \
      -DENC_SKIP_FLOATS \
      ...
```

## Example

```c
#include <stdio.h>
#include <stdint.h>
#include "config.h"

// Annotation-based exclusion
NO_ENC static int32_t public_counter = 0;

// Name patterns
static int32_t secret_key = 0xDEADBEEF;    // Matches "secret"
static int32_t api_token = 0x12345678;      // Matches neither
static int32_t debug_value = 0x11111111;    // Matches "debug"

// Different types
static float float_val = 3.14f;
static int64_t big_int = 0x123456789ABCDEF0LL;

// Array
static int32_t table[4] = {1, 2, 3, 4};

int main(void) {
    printf("counter: %d\n", public_counter);
    printf("secret:  0x%08X\n", secret_key);
    printf("token:   0x%08X\n", api_token);
    return 0;
}
```

Compile with selective encryption:

```bash
# Encrypt only "secret" variables, skip arrays
clang -fpass-plugin=path/to/libObscura.dylib \
      -DENC_FULL \
      '-DENC_ONLY_NAME="secret"' \
      -DENC_SKIP_ARRAYS \
      -I path/to/include -include config.h \
      example.c -o example
```

In this example, only `secret_key` gets encrypted.

## Filter Reference

### Blacklist Filters

| Flag | Description |
|------|-------------|
| `ENC_SKIP_NAME="pattern"` | Skip variables matching name pattern(s) |
| `ENC_SKIP_BITS="n"` | Skip variables with specified bit size(s) |
| `ENC_SKIP_FLOATS` | Skip floating-point types |
| `ENC_SKIP_INTEGERS` | Skip integer types |
| `ENC_SKIP_ARRAYS` | Skip all arrays |

### Whitelist Filters

| Flag | Description |
|------|-------------|
| `ENC_ONLY_NAME="pattern"` | Encrypt only variables matching pattern(s) |
| `ENC_ONLY_BITS="n"` | Encrypt only variables with specified bit size(s) |
| `ENC_ONLY_FLOATS` | Encrypt only floating-point types |
| `ENC_ONLY_INTEGERS` | Encrypt only integer types |

### Array Options

| Flag | Description |
|------|-------------|
| `ENC_ARRAYS_LITE_ONLY` | Arrays receive Lite encryption only |

### Annotations

| Annotation | Description |
|------------|-------------|
| `NO_ENC` | Exclude this variable from encryption |

## See Also

- [Encryption](ENCRYPTION.md) - Encryption levels and options
- [Local-to-Global Promotion](L2G.md) - Promote local constants for encryption
- [Combined Usage](COMBINED.md) - Using all features together
