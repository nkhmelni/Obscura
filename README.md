# Obscura

[![License: PolyForm Noncommercial](https://img.shields.io/badge/License-PolyForm%20Noncommercial-purple.svg)](https://polyformproject.org/licenses/noncommercial/1.0.0)

An LLVM pass plugin that obfuscates compiled code. It ships 12 passes covering constant encryption, string encryption, control flow manipulation, anti-debugging, and ObjC metadata protection (didn't I forget something?). Everything is controlled through compiler flags — no source changes required beyond including a single header.

Obscura works with any LLVM-based compiler that supports `-fpass-plugin` (Clang, AppleClang, and the Swift thing). Thus, obfuscation is effectively achieved for C(++), ObjC(++), and Swift (currently only briefly tested) runtimes. May add rustc (Rust) support in the future if any demand arises.

## How it works

LLVM's new PM lets you inject custom compiler passes (at the IR phase) into the optimization pipeline LEGALLY, without any modifications to the compiler itself. Eventually, you don't need to build LLVM anymore, and older projects like Hanabi that based on earlier versions of LLVM now render completely irrelevant, though they used to provide a great extent of convenience a while ago.

Obscura registers its passes at the `OptimizerLastEP` callback, which fires after all standard optimizations — at any `-O` level, including `-O0`. However, optimization levels other than `-O1` aren't tested so well. `-O1` is therefore recommended.

You include `config.h` in your source and pass `-D` flags to the compiler. These flags create marker globals in the IR that survive optimization. The plugin reads them and decides what to do. If no config header is included (no config marker found), a preferred set of obfuscation passes runs, for the sake of quick setup convenience.

The plugin runs in four phases:

1. **Code insertion** — Anti-ClassDump metadata protection, constant encryption, string encryption, anti-debug checks, and dynamic symbol resolution. These passes add new code to the module.
2. **Code obfuscation** — Block splitting, bogus control flow, control flow flattening, and instruction substitution. These passes obfuscate everything from phase 1 (and the original code), so decryption routines are never left in the clear.
3. **Structural obfuscation** — Indirect branching and function wrapping. These break the call graph and control flow at the module level.
4. **Cleanup** — Marker globals are removed. Nothing plugin-related survives in the final binary.

## Passes

All passes are off by default (when config.h is included). Enable them with `-D` flags and include `config.h`. It's recommended to do it globally, through a build system flag or a compiler flag.

If config.h is **not** included, the plugin applies built-in defaults automatically:

| Pass | Settings |
|------|----------|
| ACD | prob=100 |
| ADB | prob=100 |
| CONSTENC | lite only, prob=20 |
| FCO | on, hide_fw |
| STRCRY | prob=100 |
| SPLIT | num=1 |
| BCF | prob=80, loop=1, cond_compl=3, junkasm, onlyjunkasm, minnum=1, maxnum=3 |
| CFF | prob=20 |
| SUB | prob=20, loop=1 |
| INDIBRAN | prob=100 |
| L2G | prob=20, dedup |
| FUNCWRA | off |
| PRNG_SEED | 42 |

FUNCWRA is pretty useless in this combination, since it's counterproductive in relation to the effect that the FUNCWRA-less set of flags produces.

| Pass | Flag | What it does | Platform |
|------|------|--------------|----------|
| [Anti-ClassDump](docs/ACD.md) | `ENABLE_ACD` | Hides ObjC class metadata from class-dump (and other static analyzers actually) | ObjC Darwin |
| [Anti-Debug](docs/ADB.md) | `ENABLE_ADB` | Inserts ptrace-based debugger detection | AArch64 Darwin |
| [Constant Encryption](docs/CONSTENC.md) | `CONSTENC_LITE`, `CONSTENC_DEEP`, `CONSTENC_FULL` | Encrypts global constants, decrypts at runtime. Use with L2G to get more constants encrypted | All |
| [Function Call Obfuscate](docs/FCO.md) | `ENABLE_FCO` | Replaces external calls with dlopen/dlsym lookups | Darwin |
| [String Encryption](docs/STRCRY.md) | `ENABLE_STRCRY` | Encrypts string literals with per-string inline decrypt loops | All |
| [Basic Block Splitting](docs/SPLIT.md) | `ENABLE_SPLIT` | Splits blocks to create more targets for other passes | All |
| [Bogus Control Flow](docs/BCF.md) | `ENABLE_BCF` | Inserts cloned blocks behind opaque predicates | All |
| [Control Flow Flattening](docs/CFF.md) | `ENABLE_CFF` | Replaces branches with switch-based dispatch | All |
| [Instruction Substitution](docs/SUB.md) | `ENABLE_SUB` | Replaces arithmetic with equivalent but more complex expressions | All |
| [Indirect Branch](docs/INDIBRAN.md) | `ENABLE_INDIBRAN` | Converts branches to table-based indirect jumps | All |
| [Function Wrapper](docs/FUNCWRA.md) | `ENABLE_FUNCWRA` | Wraps call sites through intermediate functions | All |
| [Local-to-Global](docs/L2G.md) | `L2G_ENABLE` | Promotes local constants to globals (for encryption, has little use separately) | All |

Most passes accept probability and iteration parameters. See the individual docs or `config.h` comments for the full flag list and other details.

## Per-function annotations

You can override global settings on a per-function basis without changing `-D` flags. These are (almost) as they were in Hikari, and more annotations will be supported in the future:

```c
// enable BCF and SUB for this function only
OBSCURA_ANNOTATE("bcf bcf_prob=100 sub sub_prob=100")
int critical_function(int x) { ... }

// disable BCF for this function even if globally enabled
OBSCURA_ANNOTATE("nobcf")
int performance_sensitive(int x) { ... }

// per-function parameter tuning
OBSCURA_ANNOTATE("bcf bcf_loop=3 bcf_cond_compl=5 indibran indibran_enc_jump")
int heavily_protected(int x) { ... }
```

## Compatibility

Obscura is built against a specific LLVM version. Your compiler's LLVM version **must match** — different versions have different internal ABIs, and a mismatch will crash the compiler.

| LLVM | Status | Xcode |
|------|--------|-------|
| 19.1.5 | Stable | 26.0+ |
| 19.1.4 | Untested | 16.3 — 16.4 |
| 17.0.6 | Stable | 16.0 — 16.2 |
| 16.0.0 | Untested | 15.0 — 15.4 |

Check your version with `clang --version` (or better just open Xcode and note the version) and download the matching release. For a complete Xcode-to-LLVM version mapping, see [Wikipedia: Xcode Version History](https://en.wikipedia.org/wiki/Xcode#Xcode_15.0_-_16.x_(since_visionOS_support)).

**Languages:** C(++), Objective-C(++). The latest versions of [Swift](https://github.com/swiftlang/swift) are also supported, but this requires Xcode 26+, as per my knowledge. You may compile the newest runtime by yourself if you wish to have Swift obfuscation supported on earlier versions of Xcode. Regarding Rust support, it's completely uncertain for now, but it's at least known that it runs on LLVM, so might as well be supported by Obscura.

## Installation

Download the release for your LLVM version from [Releases](../../releases) and extract it:

```
tar -xJf obscura-llvm19.1.5-darwin-universal.tar.xz
```

You get:

```
lib/libObscura.dylib    — the plugin
lib/libDeps.dylib       — LLVM symbol fallback (macOS only)
include/config.h        — configuration header
```

Both dylibs must stay in the same directory.

## Usage

**Important:** Use `-fpass-plugin=`, not `-fplugin=`. AppleClang's `-fplugin` is a completely different thing (frontend).

### Minimal

```bash
clang -fpass-plugin=lib/libObscura.dylib -O1 file.c -o out
```

### With controlled obfuscation

```bash
clang -fpass-plugin=lib/libObscura.dylib \
    -Iinclude -include config.h \
    -DENABLE_BCF -DBCF_PROB=100 \
    -DENABLE_STRCRY \
    -DENABLE_CFF -DCFF_PROB=50 \
    -Wl,-dead_strip_dylibs -Wl,-x \
    -O1 file.c -o out
```

### Make integration

```makefile
OBSCURA := /path/to/obscura

CFLAGS += -fpass-plugin=$(OBSCURA)/lib/libObscura.dylib \
          -I$(OBSCURA)/include -include $(OBSCURA)/include/config.h \
          -DENABLE_STRCRY -DENABLE_INDIBRAN -DENABLE_SUB -DSUB_PROB=80
LDFLAGS += -Wl,-dead_strip_dylibs -Wl,-x
```

### CMake integration

```cmake
set(OBSCURA "${CMAKE_SOURCE_DIR}/obscura")

add_compile_options(
    -fpass-plugin=${OBSCURA}/lib/libObscura.dylib
    -I${OBSCURA}/include -include ${OBSCURA}/include/config.h
    -DENABLE_BCF -DBCF_PROB=100
    -DENABLE_STRCRY
    -DENABLE_CFF
)
add_link_options(-Wl,-dead_strip_dylibs -Wl,-x)
```

## Caveats

**LLVM version must match exactly.**
Not approximately — exactly. LLVM 19.1.4 and 19.1.5 have different ABIs, and AppleClang would simply crash at a point.

**Strip your binaries.**
`-Wl,-x` is recommended for all builds. Obscura already uses `PrivateLinkage` and `HiddenVisibility` to eliminate most generated symbols, but `-x` catches anything else the linker might keep around (local symbols, debug nlist entries). It's cheap and there's no reason not to. Additionally, `-Wl,-dead_strip_dylibs` is required for FCO to function properly (described in [FCO](docs/FCO.md)).

**libDeps.dylib is required on macOS.**
AppleClang doesn't export all LLVM symbols that plugins need (good job Apple). libDeps provides fallbacks. If it's missing, you'll get crashes at seemingly random points (SIGSEGV at 0x0 from unresolved lazy stubs).

**Darwin-only passes on non-Darwin = no-op.**
ADB, FCO, and ACD silently do nothing (or almost nothing) on Linux. They depend on Mach-O, the ObjC runtime, or AArch64 Darwin syscall conventions. I might add partial Linux support for these passes in the future, but it's not a serious concern for now.

**Reproducibility requires `PRNG_SEED`.**
Without a fixed seed, some passes use time-based randomization. Set `PRNG_SEED=N` for deterministic builds. Some passes might anyway be not enough deterministic even with the seed set, which should be reported.

## License

This project is licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE).

- **Permitted**: Personal use, research, education, hobby projects, and use by nonprofits
- **Required**: Attribution (see [NOTICE](NOTICE))
- **Not permitted**: Commercial use without a separate license

For commercial licensing, find a way to contact me.

See [NOTICE](NOTICE) for third-party acknowledgments.
