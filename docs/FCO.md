# Function Call Obfuscate (FCO)

Replaces direct external symbol references with runtime `dlsym` lookups and replaces ObjC class/selector references with runtime equivalents (`objc_getClass`, `sel_registerName`). After FCO, the import table and `nm -u` show only `dlsym` (and `dlopen` when framework hiding is active) - static analysis tools can't determine which system functions the binary calls without tracing the dlsym argument strings, and pairing with STRCRY encrypts those too.

The core idea (replace external calls with dlsym) comes from Hikari, but almost everything else is new - per-function inline resolvers with cached globals, compile-time symbol probing, selector stub rewriting, framework hiding, constructor/destructor skip, ACD awareness, arm64e PAC handling. Hikari's FCO wraps every external call unconditionally (including cross-TU internal symbols - silent crash), re-resolves on every function call (no caching), and only handles symbols listed in a JSON config.

Darwin only. Runs in Phase 1, after CONSTENC. Generated init blocks are normal IR inside user functions - BCF, CFF, SUB, and INDIBRAN obfuscate them automatically.

## How it works

Three-phase algorithm over the module, preceded by selector stub rewriting.

**Selector stub rewriting** runs first. Modern Apple Clang (Xcode 15+) emits `objc_msgSend$selectorName` stub declarations. The linker turns these into `__objc_stubs` code with selector strings in `__objc_methname` - a section STRCRY can't encrypt. FCO rewrites each call site: inserts `sel_registerName` with the selector string placed in `__cstring` (STRCRY-encryptable), then redirects the call to the real `objc_msgSend` or `objc_msgSendSuper2`. Dead stub declarations are erased. Selector string globals are deduplicated across call sites and both stub types.

**Phase 1** scans all eligible functions for external call sites. Each candidate symbol is validated at compile time - the pass resolves it via `dlsym` + `dladdr` inside the compiler process to verify it exists in a system library and discover which dylib provides it. Symbols not found in any system library (cross-TU internal symbols, static library symbols) are skipped entirely. This prevents the silent NULL crashes that plague Hikari's unconditional wrapping. Per-symbol cache globals (randomized names) are created for new symbols.

**Phase 2** processes each eligible function. ObjC class references become `objc_getClass` calls; selector references become `sel_registerName` calls. For functions with C external symbols, a per-function init block is built: an atomic status flag gates entry, and the init path calls `dlsym(RTLD_DEFAULT, name)` for each symbol the function uses. Resolved pointers are stored in shared cache globals. On subsequent calls, the fast path is a single atomic load + predicted branch - roughly 5-7 cycles on ARM64.

**Phase 3** (FCO_HIDE_FW only) erases original declarations whose references have all been replaced. Combined with `-Wl,-dead_strip_dylibs`, this strips framework load commands from the binary.

FCO uses `RTLD_DEFAULT` instead of `dlopen(NULL)` for dlsym lookups. This searches all loaded Mach-O images, which matters for injected dylibs (MobileSubstrate/Ellekit tweaks) where `dlopen(NULL)` only searches the host app's dependency graph.

## Framework hiding (FCO_HIDE_FW)

When enabled, init blocks `dlopen` each required framework path before calling `dlsym`. Framework paths are discovered at compile time during symbol probing (14 common frameworks pre-loaded). Always-loaded frameworks (libSystem, libobjc, libc++) are skipped.

After all operands are rewritten, Phase 3 erases the original declarations. The linker sees no undefined framework symbols, and `-Wl,-dead_strip_dylibs` removes the framework load commands. Result: `otool -L` shows only `libSystem.B.dylib`.

Known limitation: `___CFConstantStringClassReference` keeps CoreFoundation in `LC_LOAD_DYLIB`. Workaround: use `CFStringCreateWithCString` instead of `@"..."` literals.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_FCO` | off | Master switch (Darwin only) |
| `FCO_HIDE_FW` | off | Hide framework dependencies from `otool -L`. Implies `ENABLE_FCO`. Use with `-Wl,-dead_strip_dylibs` |
| `FCO_FLAG=n` | auto | Override dlopen flags. Rarely needed - the pass uses `RTLD_LAZY \| RTLD_GLOBAL` |
| `FCO_CONFIG="path"` | none | JSON file mapping symbol names to replacements. Niche feature from Hikari |

**Link your frameworks.** FCO replaces references with runtime lookups, but the linker won't complain if a framework is missing (FCO erased the undefined symbols). If you forget `-framework Security` and FCO wraps `SecItemCopyMatching`, the binary links fine but crashes silently at runtime when `dlsym` returns NULL. Always link every framework your code uses, even though the linker no longer enforces it.

**Constructors and destructors** in `llvm.global_ctors` / `llvm.global_dtors` are skipped. These run during `dlopen` when dyld holds the loader lock - `dlsym` re-enters the lock and deadlocks on macOS 15+ (dyld4). If a constructor needs to call FCO'd symbols, use `dispatch_async` to defer the call.

**ACD interaction.** When both ACD and FCO are enabled, FCO skips class reference replacement. ACD randomizes the class name registered with the runtime, so `objc_getClass("OriginalName")` would return nil. Selector replacement is unaffected. For full selector hiding, combine ACD + FCO + STRCRY.

**STRCRY pairing.** FCO alone moves symbol names from import tables to plaintext `__cstring` entries. Pair with STRCRY (`-DENABLE_STRCRY`) to encrypt those strings at rest. Without STRCRY, `strings` on the binary still reveals dlsym targets and selector names.

## Per-function annotations

```c
// Enable for a specific function
OBSCURA_ANNOTATE("fco")

// Disable for a specific function
OBSCURA_ANNOTATE("nofco")
```
