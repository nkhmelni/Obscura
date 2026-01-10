# Combined Usage

This document covers how to use encryption, filters, and L2G together, including per-module configuration for fine-grained control.

## Understanding Modules

In the context of the Encryption Pass, a **module** is a single source file being compiled. When you compile `main.c`, that's one module. When you compile `utils.c`, that's another module.

Each module is processed independently by the compiler, which means you can configure Obscura differently for each source file.

## Per-Module Configuration

You can apply different settings to different source files by using `#define` directives before including `config.h`.

### CMake

```cmake
set(OBSCURA_PLUGIN "/path/to/libObscura.dylib")
set(OBSCURA_INCLUDE "/path/to/include")

add_compile_options(
    -fpass-plugin=${OBSCURA_PLUGIN}
    -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE
    -I${OBSCURA_INCLUDE} -include ${OBSCURA_INCLUDE}/config.h
)
```

### Makefile

```makefile
OBSCURA_PLUGIN  := /path/to/libObscura.dylib
OBSCURA_INCLUDE := /path/to/include

OBSCURA_FLAGS := -fpass-plugin=$(OBSCURA_PLUGIN) \
                 -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE \
                 -I$(OBSCURA_INCLUDE) -include $(OBSCURA_INCLUDE)/config.h

CFLAGS += $(OBSCURA_FLAGS)
```

### Module-Specific Overrides

In any source file, you can override settings by placing `#define` directives before including the header:

```c
// secrets.c - Override project defaults for this file only

#define ENC_SKIP_FLOATS          // Don't encrypt floats in this module
#define ENC_ONLY_NAME "secret"   // Only encrypt variables with "secret" in name
#define L2G_PROB 50              // Only promote 50% of constants

#include "config.h"

static int32_t secret_key = 0xDEADBEEF;   // Encrypted (matches "secret")
static int32_t other_value = 0x12345678;  // Not encrypted (doesn't match)
static float secret_float = 3.14f;        // Not encrypted (floats skipped)
```

The `#define` directives in the source file take precedence over the `-D` flags from the command line for that specific module.

### Use Case: Sensitive vs. Non-Sensitive Modules

You might want maximum protection for files containing sensitive data, but lighter protection for utility code:

**sensitive_data.c:**
```c
// Maximum protection for this module
#define ENC_FULL_TIMES 10
#define ENC_DEEP_INLINE
#include "config.h"

static int32_t master_key = 0xDEADBEEF;
static int32_t encryption_iv[4] = {0x11, 0x22, 0x33, 0x44};
```

**utils.c:**
```c
// Lighter protection for utility code
#define ENC_LITE_TIMES 2
#undef ENC_DEEP  // Disable deep encryption
#include "config.h"

static int32_t buffer_size = 4096;
```

### Use Case: Excluding Specific Patterns

If your project-wide settings encrypt everything, but one module has variables you want to exclude:

**debug_helpers.c:**
```c
// Skip debug-related variables in this module
#define ENC_SKIP_NAME "debug,log,trace"
#include "config.h"

static int32_t debug_level = 3;      // Not encrypted
static int32_t log_buffer_size = 1024; // Not encrypted
static int32_t secret_key = 0xABCD;  // Encrypted (doesn't match patterns)
```

## Full Obfuscation Example

Here's a complete example using all features together:

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(MySecureApp)

set(OBSCURA_PLUGIN "/path/to/libObscura.dylib")
set(OBSCURA_INCLUDE "/path/to/include")

add_compile_options(
    -fpass-plugin=${OBSCURA_PLUGIN}
    -DENC_FULL -DENC_FULL_TIMES=5 -DENC_DEEP_INLINE
    -DL2G_ENABLE -DL2G_OPS
    -I${OBSCURA_INCLUDE} -include ${OBSCURA_INCLUDE}/config.h
)

add_executable(myapp main.c crypto.c utils.c)
```

### main.c

```c
#include <stdio.h>
#include <stdint.h>
#include "config.h"

// Global encrypted data
static int32_t app_version = 0x010203;

int main(void) {
    // L2G promotes this constant
    int32_t magic = 0xCAFEBABE;

    printf("Version: %d.%d.%d\n",
           (app_version >> 16) & 0xFF,
           (app_version >> 8) & 0xFF,
           app_version & 0xFF);
    printf("Magic: 0x%08X\n", magic);
    return 0;
}
```

### crypto.c

```c
// Maximum protection for crypto module
#define ENC_FULL_TIMES 10
#include "config.h"

static int32_t secret_key = 0xDEADBEEF;
static int32_t key_schedule[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};

int32_t encrypt_block(int32_t data) {
    L2G int32_t round_constant = 0x9E3779B9;
    return data ^ secret_key ^ round_constant;
}
```

### utils.c

```c
// Lighter encryption for utilities, skip arrays
#define ENC_LITE_TIMES 2
#define ENC_SKIP_ARRAYS
#include "config.h"

static int32_t buffer_sizes[4] = {256, 512, 1024, 2048};  // Not encrypted
static int32_t default_timeout = 30000;  // Encrypted

NO_ENC static int32_t debug_flag = 0;  // Never encrypted

int get_buffer_size(int level) {
    return buffer_sizes[level];
}
```

## Common Recipes

### Maximum Protection

```bash
clang ... \
    -DENC_FULL \
    -DENC_FULL_TIMES=10 \
    -DENC_DEEP_INLINE \
    -DL2G_ENABLE \
    -DL2G_OPS \
    ...
```

### Balanced Protection

```bash
clang ... \
    -DENC_FULL \
    -DENC_FULL_TIMES=3 \
    -DL2G_ENABLE \
    -DENC_ARRAYS_LITE_ONLY \
    ...
```

### Selective Protection (Secrets Only)

```bash
clang ... \
    -DENC_FULL \
    '-DENC_ONLY_NAME="secret,key,password,token"' \
    ...
```

### Integers Only (Fast)

```bash
clang ... \
    -DENC_LITE \
    -DENC_ONLY_INTEGERS \
    -DENC_SKIP_ARRAYS \
    ...
```

### L2G Without Encryption

Use L2G for code complexity without the performance cost of encryption:

```c
#define L2G_ENABLE
// Don't define any ENC_* flags
#include "config.h"

void process(void) {
    int32_t multiplier = 42;  // Promoted to global, but not encrypted
}
```

## How Features Interact

### L2G + Encryption

When both are enabled:
1. L2G promotes local constants to globals
2. Encryption encrypts all eligible globals (including promoted ones)

### L2G + Filters

Filters apply to all globals, including those promoted by L2G:

```c
#define L2G_ENABLE
#define ENC_FULL
#define ENC_ONLY_INTEGERS
#include "config.h"

void process(void) {
    int32_t int_val = 42;    // Promoted AND encrypted
    float float_val = 3.14f; // Promoted but NOT encrypted (filtered)
}
```

### Annotations

Annotations provide the finest control:

| Combination | Result |
|-------------|--------|
| `L2G` | Promoted, encrypted (if encryption enabled) |
| `NO_L2G` | Not promoted (stays local) |
| `NO_ENC` | Not encrypted (but may still be promoted) |
| `L2G NO_ENC` | Promoted but not encrypted |

## Troubleshooting

### Variables Not Being Encrypted

1. Check if `config.h` is included (implicit mode runs without it)
2. Check if a filter is excluding the variable
3. Check if `NO_ENC` is applied
4. For locals, check if L2G is enabled or `L2G` annotation is used

### Too Much Code Bloat

1. Reduce `ENC_*_TIMES` values
2. Disable `ENC_DEEP_INLINE`
3. Use `ENC_SKIP_ARRAYS` or `ENC_ARRAYS_LITE_ONLY`
4. Lower `L2G_PROB` or disable `L2G_OPS`

### Disassembler Failures

High `ENC_DEEP_TIMES` with `ENC_DEEP_INLINE` can cause disassemblers like IDA to fail pseudocode generation. This is actually a feature for protection, but can complicate debugging. Use lower values during development.

## Flag Reference

All available `-D` flags:

### Encryption Levels

| Flag | Description |
|------|-------------|
| `-DENC_LITE` | Enable Lite encryption (XOR-based) |
| `-DENC_DEEP` | Enable Deep encryption (salt-based) |
| `-DENC_FULL` | Enable both Lite and Deep |

### Encryption Options

| Flag | Description |
|------|-------------|
| `-DENC_LITE_TIMES=n` | Lite encryption iterations (1-15) |
| `-DENC_DEEP_TIMES=n` | Deep encryption iterations (1-15) |
| `-DENC_FULL_TIMES=n` | Set both iteration counts |
| `-DENC_DEEP_INLINE` | Inline Deep decryption code |

### Filters (Blacklist)

| Flag | Description |
|------|-------------|
| `-DENC_SKIP_NAME="pattern"` | Skip variables matching pattern(s) |
| `-DENC_SKIP_BITS="n"` | Skip variables with bit size(s) |
| `-DENC_SKIP_FLOATS` | Skip floating-point types |
| `-DENC_SKIP_INTEGERS` | Skip integer types |
| `-DENC_SKIP_ARRAYS` | Skip all arrays |

### Filters (Whitelist)

| Flag | Description |
|------|-------------|
| `-DENC_ONLY_NAME="pattern"` | Encrypt only matching pattern(s) |
| `-DENC_ONLY_BITS="n"` | Encrypt only specified bit size(s) |
| `-DENC_ONLY_FLOATS` | Encrypt only floating-point types |
| `-DENC_ONLY_INTEGERS` | Encrypt only integer types |

### Array Options

| Flag | Description |
|------|-------------|
| `-DENC_ARRAYS_LITE_ONLY` | Arrays receive Lite encryption only |

### L2G (Local-to-Global)

| Flag | Description |
|------|-------------|
| `-DL2G_ENABLE` | Enable automatic promotion |
| `-DL2G_INTEGERS=0/1` | Control integer promotion |
| `-DL2G_FLOATS=0/1` | Control float promotion |
| `-DL2G_INT_ARRAYS=0/1` | Control integer array promotion |
| `-DL2G_FLOAT_ARRAYS=0/1` | Control float array promotion |
| `-DL2G_OPS` | Promote binary operation results |
| `-DL2G_DEDUP` | Deduplicate identical constants |
| `-DL2G_PROB=n` | Promotion probability (0-100) |
| `-DL2G_MAX_ARRAY=n` | Maximum array size |

> **Note**
>
> For flags with string values (like `ENC_SKIP_NAME`), wrap them in single quotes in CMake to preserve the inner quotes.

## See Also

- [Encryption](ENCRYPTION.md) - Encryption levels and options
- [Filters](FILTERS.md) - Control which variables get encrypted
- [Local-to-Global Promotion](L2G.md) - Promote local constants for encryption
