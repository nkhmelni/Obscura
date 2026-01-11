# Obscura

[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-orange.svg)](https://creativecommons.org/licenses/by-nc/4.0/)

An LLVM pass plugin for variable encryption and subsequent runtime decryption. Protects sensitive data from static (and dynamic) analysis while maintaining full functionality, with manageable performance overhead.

## Features

- **Two-level encryption** - Lightweight (XOR-based) and Deep (salt-based) encryption
- **Flexible configuration** - Fine-grained control via compiler flags
- **Local-to-Global promotion** - Extends protection to local constants
- **Per-variable control** - Annotations to include or exclude specific variables
- **Filter system** - Whitelist/blacklist by name, type, or size

## Compatibility

Obscura is built against a specific LLVM version. Your compiler's LLVM version must match the build version for correct operation.

**Why this matters:** Different LLVM versions have slightly different internal ABIs. If the versions don't match, symbols may be resolved incorrectly, leading to crashes or undefined behavior.

### Available Versions

Obscura is released for multiple LLVM versions:

| LLVM Version | Status |
|--------------|--------|
| 19.1.5 | Latest |
| 19.1.4 | Stable |
| 17.0.6 | Stable (tested with AppleClang 16.0.0) |
| 16.0.0 | Stable |

**Languages:** C, C++, Objective-C, Objective-C++

Other LLVM-based compilers that support plugins (e.g., Swift, Rust) may also work if they use a compatible LLVM version—this is untested and left for you to explore.

### Xcode Users

Xcode bundles a specific LLVM version. Check the release filename for the LLVM version, then verify your Xcode version is compatible:

| Xcode Version | LLVM Version |
|---------------|--------------|
| 26.0 - 26.2 | 19.1.5 |
| 16.3 - 16.4 | 19.1.4 |
| 16.0 - 16.2 | 17.0.6 |
| 15.0 - 15.4 | 16.0.0 |

For a complete Xcode-to-LLVM version mapping, see: [Wikipedia: Xcode Version History](https://en.wikipedia.org/wiki/Xcode#Xcode_15.0_-_16.x_(since_visionOS_support))

### Non-Xcode Users

If you're using a standalone Clang/LLVM installation, check your compiler's LLVM version:

```bash
clang --version
```

Download the release that matches your LLVM version. Using a mismatched version will result in undefined behavior.

## Installation

1. Download the release matching your LLVM version from [Releases](../../releases)
2. Extract the archive:
   ```bash
   tar -xJf obscura-llvm<VERSION>-<ARCH>.tar.xz
   # Example: tar -xJf obscura-llvm17.0.6-arm64.tar.xz
   ```
3. You'll have:
   ```
   lib/
   ├── libObscura.dylib
   └── libDeps.dylib
   include/
   └── config.h
   ```

Both `.dylib` files must be in the same directory.

## Quick Start

### Minimal Usage (Implicit Mode)

Without including `config.h`, full encryption runs automatically:

```bash
clang -fpass-plugin=/path/to/libObscura.dylib program.c -o program
```

This applies full encryption with default settings (Lite + Deep, 1 iteration each, no inlining). Filters and L2G are not available in this mode.

### Explicit Mode (Recommended)

Include `config.h` for full control:

```bash
clang -fpass-plugin=/path/to/libObscura.dylib \
      -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE \
      -I /path/to/include -include /path/to/include/config.h \
      program.c -o program
```

### Build System Integration

```cmake
# CMake
set(OBSCURA_PLUGIN "/path/to/libObscura.dylib")
set(OBSCURA_INCLUDE "/path/to/include")

add_compile_options(
    -fpass-plugin=${OBSCURA_PLUGIN}
    -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE
    -I${OBSCURA_INCLUDE} -include ${OBSCURA_INCLUDE}/config.h
)
```

See [Combined Usage](docs/COMBINED.md) for Makefile integration and flag reference.

## Documentation

| Document | Description |
|----------|-------------|
| [Encryption](docs/ENCRYPTION.md) | Encryption levels, iterations, and inline mode |
| [Filters](docs/FILTERS.md) | Whitelist/blacklist filters and `NO_ENC` annotation |
| [L2G](docs/L2G.md) | Local-to-Global promotion with `L2G` and `NO_L2G` |
| [Combined Usage](docs/COMBINED.md) | Per-module configuration and CMake integration |

## Overview

### Encryption Levels

| Level | Flag | Description |
|-------|------|-------------|
| Lite | `ENC_LITE` | Fast XOR-based encryption |
| Deep | `ENC_DEEP` | Salt-based encryption with function call |
| Full | `ENC_FULL` | Both levels combined |

### Key Options

| Option | Description |
|--------|-------------|
| `ENC_FULL_TIMES=n` | Apply encryption n times (1-15) |
| `ENC_DEEP_INLINE` | Inline decryption at each use site |
| `L2G_ENABLE` | Promote local constants to globals |

### Annotations

Use these in your code for per-variable control:

```c
#include "config.h"

NO_ENC static int32_t public_val = 1;   // Never encrypted

void func(void) {
    L2G int32_t secret = 0xDEAD;        // Promoted and encrypted
    NO_L2G int32_t counter = 0;         // Not promoted
    L2G NO_ENC int32_t val = 42;        // Promoted but not encrypted
}
```

## Example

```c
#include <stdio.h>
#include <stdint.h>
#include "config.h"

static int32_t secret_key = 0xDEADBEEF;
NO_ENC static int32_t version = 0x0102;

int main(void) {
    L2G int32_t magic = 0xCAFEBABE;
    printf("Key: 0x%08X\n", secret_key);
    printf("Magic: 0x%08X\n", magic);
    return 0;
}
```

The program runs correctly, but static analysis tools won't see `0xDEADBEEF` or `0xCAFEBABE` in the binary.

## Sample Project

See the [sample/](sample/) directory for a complete working example with CMake.

## License

This project is licensed under [CC BY-NC 4.0](LICENSE) (Creative Commons Attribution-NonCommercial 4.0).

- **Permitted**: Personal use, research, education, non-commercial projects
- **Required**: Attribution
- **Not permitted**: Commercial use
