# Obscura

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

### Tested Configuration

The current release was tested with:
- **LLVM 17.0.6**
- **AppleClang 16.0.0** (clang-1600.0.26.4)
- **Languages:** C, C++, Objective-C, Objective-C++

Other LLVM-based compilers that support plugins (e.g., Swift, Rust) may also work if they use a compatible LLVM version—this is untested and left for you to explore.

### Xcode Users

Xcode bundles a specific LLVM version. Check the release filename for the LLVM version, then verify your Xcode version is compatible:

| Xcode Version | LLVM Version |
|---------------|--------------|
| 16.0 - 16.2 | LLVM 17.0.6 |

For a complete Xcode-to-LLVM version mapping, see: [Wikipedia: Xcode Version History](https://en.wikipedia.org/wiki/Xcode#Xcode_7.0_-_11.x_(since_Free_On-Device_Development))

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
   tar -xzf obscura-llvm17.0.6-arm64.tar.gz
   ```
3. You'll have:
   ```
   obscura-llvm17.0.6-arm64/
   ├── lib/
   │   ├── libObscura.dylib
   │   └── libDeps.dylib
   └── include/
       └── enc_options.h
   ```

Both `.dylib` files must be in the same directory.

## Quick Start

### Minimal Usage (Implicit Mode)

Without including `enc_options.h`, full encryption runs automatically:

```bash
clang -fpass-plugin=/path/to/libObscura.dylib program.c -o program
```

This applies full encryption with default settings (Lite + Deep, 1 iteration each, no inlining). Filters and L2G are not available in this mode.

### Explicit Mode (Recommended)

Include `enc_options.h` for full control:

```bash
clang -fpass-plugin=/path/to/libObscura.dylib \
      -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE \
      -I /path/to/include -include /path/to/include/enc_options.h \
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
    -I${OBSCURA_INCLUDE} -include ${OBSCURA_INCLUDE}/enc_options.h
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
#include "enc_options.h"

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
#include "enc_options.h"

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

This project is licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE).

- **Permitted**: Personal use, research, education, hobby projects
- **Not permitted**: Commercial use

For commercial licensing inquiries, please contact the author.
