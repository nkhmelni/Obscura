# Local-to-Global Promotion (L2G)

L2G promotes local constants to global variables, making them available for encryption and increasing code complexity for reverse engineering.

## Why L2G?

The Encryption Pass only encrypts global variables. Local constants inside functions are typically embedded directly in the code or placed in read-only sections without encryption.

L2G solves this by:
1. Identifying local constants in your functions
2. Promoting them to global variables
3. Making them eligible for encryption

This significantly increases obfuscation coverage.

## How It Works

L2G operates in two modes:

| Mode | Trigger | Behavior |
|------|---------|----------|
| **Marker-based** | `L2G` annotation | Specific variables are always promoted |
| **Automatic** | `L2G_ENABLE` flag | All eligible constants are promoted |

### Marker-Based Promotion

Use the `L2G` annotation to mark specific variables for promotion:

```c
#include "config.h"

void process(void) {
    L2G int32_t secret = 0xDEADBEEF;  // Always promoted
    int32_t normal = 42;               // Not promoted (no L2G_ENABLE)
}
```

Marker-based promotion works even without the `L2G_ENABLE` flag.

### Automatic Promotion

Enable automatic promotion with `L2G_ENABLE`:

```bash
clang ... -DL2G_ENABLE -I path/to/include -include config.h ...
```

With automatic promotion, all eligible local constants are promoted unless excluded with `NO_L2G`:

```c
#include "config.h"

void process(void) {
    int32_t secret = 0xDEADBEEF;       // Promoted automatically
    NO_L2G int32_t counter = 0;        // Excluded from automatic promotion
}
```

## Annotations

| Annotation | Description |
|------------|-------------|
| `L2G` | Mark a local variable for promotion (always promoted) |
| `NO_L2G` | Exclude a local variable from automatic promotion |

### Combining Annotations

Annotations can be combined with `NO_ENC` in any order:

```c
L2G NO_ENC int32_t val = 42;  // Promoted but NOT encrypted
NO_ENC L2G int32_t val = 42;  // Same effect
```

This is useful when you want the obfuscation benefits of promotion without the performance cost of encryption.

## Type Control

Control which types are automatically promoted:

| Flag | Default | Description |
|------|---------|-------------|
| `L2G_INTEGERS=0/1` | 1 (on) | Promote integer constants |
| `L2G_FLOATS=0/1` | 1 (on) | Promote float constants |
| `L2G_INT_ARRAYS=0/1` | 1 (on) | Promote integer arrays |
| `L2G_FLOAT_ARRAYS=0/1` | 1 (on) | Promote float arrays |

With `L2G_ENABLE`, all types are enabled by default. Use `=0` to disable specific types:

```bash
# Promote everything except floats
clang ... -DL2G_ENABLE -DL2G_FLOATS=0 ...

# Promote only integers (no floats, no arrays)
clang ... -DL2G_ENABLE -DL2G_FLOATS=0 -DL2G_INT_ARRAYS=0 -DL2G_FLOAT_ARRAYS=0 ...
```

## Options

### Operation Promotion

| Flag | Description |
|------|-------------|
| `L2G_OPS` | Promote results of binary operations |

When enabled, results of operations like `a + b` or `x * y` can also be promoted:

```bash
clang ... -DL2G_ENABLE -DL2G_OPS ...
```

### Deduplication

| Flag | Description |
|------|-------------|
| `L2G_DEDUP` | Deduplicate identical constants |

When enabled, identical constants across a function share the same global variable:

```bash
clang ... -DL2G_ENABLE -DL2G_DEDUP ...
```

### Probability

| Flag | Description |
|------|-------------|
| `L2G_PROB=n` | Promotion probability (0-100, default: 100) |

Control what percentage of eligible constants get promoted:

```bash
# Promote 50% of eligible constants
clang ... -DL2G_ENABLE -DL2G_PROB=50 ...
```

This can help balance obfuscation with code size.

### Array Size Limit

| Flag | Description |
|------|-------------|
| `L2G_MAX_ARRAY=n` | Maximum array size to promote (default: 1024, 0=unlimited) |

Large arrays can bloat the global section. Limit promotion to smaller arrays:

```bash
# Only promote arrays with 16 or fewer elements
clang ... -DL2G_ENABLE -DL2G_MAX_ARRAY=16 ...
```

## Example

```c
#include <stdio.h>
#include <stdint.h>
#include "config.h"

int process_data(void) {
    // Annotation-based: always promoted
    L2G int32_t key = 0xDEADBEEF;

    // Promoted but not encrypted
    L2G NO_ENC int32_t public_key = 0x12345678;

    // Excluded from automatic promotion
    NO_L2G int32_t counter = 0;

    // Automatic promotion (if L2G_ENABLE)
    int32_t multiplier = 42;
    float threshold = 3.14f;
    int32_t table[4] = {1, 2, 3, 4};

    int32_t result = key ^ (multiplier * table[counter]);
    return (float)result > threshold ? 1 : 0;
}

int main(void) {
    printf("Result: %d\n", process_data());
    return 0;
}
```

### Compile with Annotations Only

```bash
clang -fpass-plugin=path/to/libObscura.dylib \
      -DENC_FULL \
      -I path/to/include -include config.h \
      example.c -o example
```

Only `key` and `public_key` (the annotated variables) are promoted. `public_key` is promoted but not encrypted due to `NO_ENC`.

### Compile with Automatic Promotion

```bash
clang -fpass-plugin=path/to/libObscura.dylib \
      -DL2G_ENABLE -DENC_FULL \
      -I path/to/include -include config.h \
      example.c -o example
```

All eligible constants are promoted and encrypted, except:
- `counter` (excluded by `NO_L2G`)
- `public_key` (promoted but not encrypted due to `NO_ENC`)

## L2G Reference

### Core Flags

| Flag | Description |
|------|-------------|
| `L2G_ENABLE` | Enable automatic promotion |

### Type Control

| Flag | Description |
|------|-------------|
| `L2G_INTEGERS=0/1` | Control integer promotion |
| `L2G_FLOATS=0/1` | Control float promotion |
| `L2G_INT_ARRAYS=0/1` | Control integer array promotion |
| `L2G_FLOAT_ARRAYS=0/1` | Control float array promotion |

### Options

| Flag | Description |
|------|-------------|
| `L2G_OPS` | Promote binary operation results |
| `L2G_DEDUP` | Deduplicate identical constants |
| `L2G_PROB=n` | Promotion probability (0-100) |
| `L2G_MAX_ARRAY=n` | Maximum array size (0=unlimited) |

### Annotations

| Annotation | Description |
|------------|-------------|
| `L2G` | Mark for promotion |
| `NO_L2G` | Exclude from automatic promotion |

## See Also

- [Encryption](ENCRYPTION.md) - Encryption levels and options
- [Filters](FILTERS.md) - Control which variables get encrypted
- [Combined Usage](COMBINED.md) - Using all features together
