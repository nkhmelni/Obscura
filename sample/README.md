# Sample Project

This sample demonstrates how to integrate the Encryption Pass into your project.

## Prerequisites

1. Download the Encryption Pass release for your LLVM version
2. Extract `libEncryption.dylib`, `libEncDeps.dylib`, and `enc_options.h`

## Building

### With CMake

1. Edit `CMakeLists.txt` and set `ENC_PLUGIN` and `ENC_INCLUDE` to your paths:

```cmake
set(ENC_PLUGIN "/path/to/libEncryption.dylib" CACHE PATH "...")
set(ENC_INCLUDE "/path/to/include" CACHE PATH "...")
```

2. Build:

```bash
mkdir build && cd build
cmake ..
make
```

### Direct Compilation

```bash
clang -fpass-plugin=/path/to/libEncryption.dylib \
      -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE \
      -DL2G_ENABLE \
      -I /path/to/include -include /path/to/include/enc_options.h \
      main.c -o sample
```

## Running

```bash
./sample
```

Expected output:

```
=== Encryption Pass Sample ===

Secret Key:  0xDEADBEEF
API Token:   0x12345678
Magic Ratio: 3.141590

Lookup Sum:  0x240

Version:     1.2.3 (not encrypted)
Debug Level: 0 (not encrypted)

L2G auto:    0x...
L2G marker:  0x...

--- Type Demonstrations ---
int8:   0x12
int16:  0x1234
int32:  0x12345678
int64:  0x123456789ABCDEF0
float:  2.718280
double: 1.414210
```

The values appear correct at runtime, but if you examine the binary with a disassembler, the original values won't be visible.

## What's Demonstrated

- **Global encryption**: `secret_key`, `api_token`, `magic_ratio`, `lookup_table`
- **NO_ENC annotation**: `public_version`, `debug_level` are not encrypted
- **Automatic L2G**: Local constants in `process_with_l2g()` are promoted and encrypted
- **L2G markers**: Explicit `L2G` annotation in `process_with_marker()`
- **Combined annotations**: `L2G NO_ENC` promotes but doesn't encrypt
- **NO_L2G**: Excludes specific locals from automatic promotion

## Customization

Modify the flags in `CMakeLists.txt` to experiment:

```cmake
# Try different encryption levels
-DENC_LITE                    # Lite only
-DENC_DEEP                    # Deep only
-DENC_FULL                    # Both

# Adjust iterations
-DENC_FULL_TIMES=10           # More iterations

# Add filters
'-DENC_ONLY_NAME="secret"'    # Only encrypt "secret" variables
-DENC_SKIP_ARRAYS             # Skip array encryption

# L2G options
-DL2G_PROB=50                 # 50% promotion probability
-DL2G_DEDUP                   # Deduplicate constants
```

See the [documentation](../docs/) for full details.
