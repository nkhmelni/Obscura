# Function Call Obfuscate (FCO)

Replaces direct external symbol references with runtime `dlsym` lookups, replaces ObjC class/selector metadata with runtime calls, and optionally replaces any import with a user-defined wrapper via `FCO_MAP`. After FCO, the import table shows only `dlsym` — static analysis tools can't determine which system functions the binary calls without tracing the dlsym argument strings, and pairing with STRCRY encrypts those too.

The core idea (replace external calls with dlsym) comes from Hikari, but almost everything else is new — per-function inline resolvers with cached globals, compile-time symbol probing, selector stub rewriting, framework hiding, constructor/destructor skip, ACD awareness, arm64e PAC handling, and the general-purpose replacement map.

Darwin only. Runs in Phase 1, after CONSTENC. Generated init blocks are normal IR inside user functions — BCF, CFF, SUB, and INDIBRAN obfuscate them automatically.

## How it works

Four-stage algorithm over the module.

**Selector stub rewriting** runs first. Modern Apple Clang (Xcode 14+) emits `objc_msgSend$selectorName` stub declarations. The linker turns these into `__objc_stubs` code with selector strings in `__objc_methname` — a section STRCRY can't encrypt. FCO rewrites each call site: inserts `sel_registerName` with the selector string placed in `__cstring` (STRCRY-encryptable), then redirects the call to the real `objc_msgSend` or `objc_msgSendSuper2`. Dead stub declarations are erased. Selector string globals are deduplicated across call sites and both stub types. On typed-pointer LLVM (≤14), calls through bitcast ConstantExprs are walked to find the actual call sites.

**General replacements** (`FCO_MAP` non-runtime entries) run next. For each eligible function, calls to mapped external functions are replaced with calls to the user's wrapper. The wrapper's own body is excluded from its own replacement to prevent infinite recursion — the wrapper's internal call to the original goes through FCO's normal dlsym path. Original declarations are erased if they become use-empty. Not gated by `FCO_PROB` — general replacements always apply.

**Phase 1** scans all eligible functions for external call sites. Each candidate symbol is validated at compile time — the pass resolves it via `dlsym` + `dladdr` inside the compiler process to verify it exists in a system library and discover which dylib provides it. Symbols not found in any system library (cross-TU internal symbols, static library symbols) are skipped entirely. This prevents the silent NULL crashes that plague Hikari's unconditional wrapping. Per-symbol cache globals (randomized names) and shared dlsym argument strings are created for new symbols.

**Phase 2** processes each eligible function. ObjC class references become `objc_getClass` calls; selector references become `sel_registerName` calls. For functions with C external symbols, a per-function init block is built: an atomic status flag gates entry, and the init path calls `dlsym(RTLD_DEFAULT, name)` for each symbol the function uses. Resolved pointers are stored in shared cache globals. On subsequent calls, the fast path is a single atomic load + predicted branch — roughly 5-7 cycles on ARM64.

**Phase 3** (FCO_HIDE_FW only) erases original declarations whose references have all been replaced. Combined with `-Wl,-dead_strip_dylibs`, this strips framework load commands from the binary.

FCO uses `RTLD_DEFAULT` instead of `dlopen(NULL)` for dlsym lookups. This searches all loaded Mach-O images, which matters for injected dylibs (MobileSubstrate/Ellekit tweaks) where `dlopen(NULL)` only searches the host app's dependency graph.

## FCO_MAP

Semicolon-separated `original=replacement` entries. Define each wrapper in exactly one source file (any language: C, C++, ObjC, ObjC++). No `extern "C"` needed — the pass resolves C++ mangled names automatically via Itanium prefix scan and cross-TU alias creation. Use C-visible function names (not MachO symbols). Works with any cross-library imports (either system runtime functions or something arbitrary).

```c
// wrappers.c (or .m, .mm, .cpp)
#include <dlfcn.h>
#include <objc/runtime.h>

void *my_dlsym(void *h, const char *s) { return dlsym(h, s); }
void *my_selreg(const char *n) { return (void *)sel_registerName(n); }
int my_open(const char *p, int f, ...) { return open(p, f); }
```

```
-DFCO_MAP="dlsym=my_dlsym;sel_registerName=my_selreg;open=my_open"
```

Implies `ENABLE_FCO`. Wrappers must have the same signature as the original.

## Framework hiding (FCO_HIDE_FW)

When enabled, init blocks `dlopen` each required framework path before calling `dlsym`. Framework paths are discovered at compile time during symbol probing (14 common frameworks pre-loaded). Always-loaded frameworks (libSystem, libobjc, libc++) are skipped.

After all operands are rewritten, Phase 3 erases the original declarations. The linker sees no undefined framework symbols, and `-Wl,-dead_strip_dylibs` removes the framework load commands. Result: `otool -L` shows only `libSystem.B.dylib`.

Known limitation: `___CFConstantStringClassReference` keeps CoreFoundation in `LC_LOAD_DYLIB`. Workaround: use `CFStringCreateWithCString` instead of `@"..."` literals.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_FCO` | off | Master switch (Darwin only) |
| `FCO_PROB=n` | 100 | Per-function probability (0-100) for dlsym conversion. Does not affect selector stubs, ObjC metadata, or `FCO_MAP` general replacements |
| `FCO_HIDE_FW` | off | Hide framework dependencies from `otool -L`. Implies `ENABLE_FCO`. Use with `-Wl,-dead_strip_dylibs` |
| `FCO_MAP="..."` | none | Replacement map. Implies `ENABLE_FCO` |
| `FCO_CONFIG="path"` | none | JSON file mapping symbol names to replacements (Hikari compatibility) |

**Link your frameworks.** FCO replaces references with runtime lookups, but the linker won't complain if a framework is missing. If you forget `-framework Security` and FCO wraps `SecItemCopyMatching`, the binary links fine but crashes silently at runtime when `dlsym` returns NULL. Always link every framework your code uses, even though the linker no longer enforces it.

**Constructors and destructors** in `llvm.global_ctors` / `llvm.global_dtors` are skipped for dlsym conversion — `dlsym` re-enters the dyld loader lock and deadlocks on macOS 15+ (dyld4). `FCO_MAP` general replacements still apply in constructors (no dlsym involved). Use `dispatch_async` if a constructor needs to call dlsym-wrapped symbols.

**ACD interaction.** When both ACD and FCO are enabled, FCO skips class reference replacement. ACD randomizes the class name registered with the runtime, so `objc_getClass("OriginalName")` would return nil. Selector replacement is unaffected. For full selector hiding, combine ACD + FCO + STRCRY.

**STRCRY pairing.** FCO alone moves symbol names from import tables to plaintext `__cstring` entries. Pair with STRCRY (`-DENABLE_STRCRY`) to encrypt those strings at rest. Without STRCRY, `strings` on the binary still reveals dlsym targets and selector names.

## Per-function annotations

```c
// Enable/disable FCO for this function
OBSCURA_ANNOTATE("fco")
OBSCURA_ANNOTATE("nofco")

// Override probability
OBSCURA_ANNOTATE("fco fco_prob=80")
```

`nofco` disables all FCO processing for the function — dlsym conversion, ObjC metadata replacement, and general replacements.
