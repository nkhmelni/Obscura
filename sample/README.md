# Sample Project

Demonstrates Obscura integration.

## Prerequisites

Download and extract the Obscura release for your LLVM version.

## Building

### CMake

Edit `CMakeLists.txt` to set `OBSCURA_PLUGIN` and `OBSCURA_INCLUDE`, then:

```bash
mkdir build && cd build
cmake .. && make
```

### Direct

```bash
clang -fpass-plugin=/path/to/libObscura.dylib \
      -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE \
      -I /path/to/include -include /path/to/include/config.h \
      main.c -o sample
```

## What's Demonstrated

- Global encryption (`secret_key`, `api_token`, `lookup_table`)
- `NO_ENC` annotation (`public_version`, `debug_level`)
- Automatic L2G (`process_with_l2g()`)
- `L2G` markers (`process_with_marker()`)
- Combined `L2G NO_ENC`
- `NO_L2G` exclusion

See the [documentation](../docs/) for details.
