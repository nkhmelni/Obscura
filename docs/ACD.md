# Anti-ClassDump (ACD)

Strips ObjC class metadata from the binary and re-registers everything dynamically at load time. After ACD, class-dump recovers nothing - `class_copyMethodList`, `class_copyIvarList`, `class_copyPropertyList`, and `protocol_copyMethodDescriptionList` all return empty. The methods, ivars, and properties still work at runtime because the ObjC runtime uses pointer-based dispatch (SEL->IMP tables, `OBJC_IVAR_$_` offset globals), not the metadata lists.

The core idea (null method lists, re-register in +load via `class_replaceMethod`) comes from Hikari, but almost everything else is new - class name randomization, protocol handling, category handling, ivar/property nulling, type encoding sanitization, symbol hiding, arm64e ptrauth management. Hikari's IMP rename uses the literal string `"ACDMethodIMP"` (obvious fingerprint) and its `+initialize` mode has race conditions and category override vulnerabilities. ACD in Obscura is +load only and a full rewrite.

Runs in Phase 0, before all other passes. Generated +load functions are normal IR - BCF, CFF, SUB, and INDIBRAN obfuscate them automatically.

**Note:** Though ACD was initially aimed at protecting from class-dump, now its purpose is much broader, and it produces great impact on other static analysis tools like IDA.

## How it works

Per class, ACD creates (or injects into) a `+load` function that calls `class_replaceMethod` for every instance and class method, then nulls `baseMethods`, `ivars`, and `baseProperties` in `class_ro_t`. The class name is replaced in-place with a same-length hex string derived from `fnv1a(className, prngSeed)` - not the shared PRNG, which reseeds per module and would produce collisions across TUs. `OBJC_CLASS_$_` and `OBJC_METACLASS_$_` get `HiddenVisibility` (maps to `N_PEXT` in Mach-O - invisible in export trie, stripped by `strip -x`, but still allows cross-TU linking within the same binary). All method IMP functions are renamed to `.acimp.XXXXXX` with `PrivateLinkage` - gone from the symbol table entirely.

A minimal +load-only method list is installed in the metaclass so the runtime discovers the class as non-lazy. `+load` is called via direct IMP by the runtime (not `objc_msgSend`), so it's immune to category overrides and fires during image loading before any constructor or user code.

Module-wide, ACD also:

- Randomizes protocol names (same hex technique, in-place) and nulls protocol method list fields (3-7, 10). Protocol conformance still works - `conformsToProtocol:` uses pointer comparison, not names.
- Randomizes category names, nulls category instance methods and properties. For categories with `+load`: the class method list is trimmed in-place to `+load` only; other selectors are collected as dead.
- Sets `HiddenVisibility` on `OBJC_IVAR_$_` symbols for processed classes (brace-declared ivars have `ExternalLinkage` by default and survive `strip -x`).
- Strips `@"ClassName"` patterns to bare `@` in type encoding strings across `__objc_methtype`, `__objc_methname`, and block type encodings.
- Cleans up dead method list GVs, selector name GVs, type encoding GVs, and ptrauth wrappers from `llvm.compiler.used`.

## What breaks

ACD intentionally breaks runtime introspection. `class_getName` returns a random hex string. `NSClassFromString(@"OriginalName")` returns nil - the class is registered under the hex name. Use `ACD_CLASS_ALIAS` to register the original name as an alias (transparent empty subclass, ~400 bytes each), or exclude the class with `noacd`.

KVC `valueForKey:` fallback to direct ivar access uses the ivar list, which is nulled. Property-backed KVC through getters/setters still works (ACD re-registers them as methods). `NSCoding` / `NSKeyedArchiver` uses `class_getName` for the archived class name - unarchiving with a different `PRNG_SEED` (or without ACD) fails to find the class.

## FCO interaction

When both ACD and FCO are enabled, FCO skips `OBJC_CLASSLIST_REFERENCES` replacement. FCO's `HandleObjC` would call `objc_getClass("OriginalName")`, which returns nil after ACD randomizes the name. Selector stub rewriting is unaffected.

For full selector hiding, combine ACD + FCO + STRCRY. ACD's `class_replaceMethod` calls put selector strings in `__cstring` (STRCRY-encryptable). FCO's selector stub rewriting moves call-site selectors from `__objc_methname` (unencryptable) to `__cstring`. STRCRY encrypts both.

## arm64e

On arm64e, ACD handles pointer authentication for all method registration and +load installation. IMP pointers, method lists, and generated functions all carry the correct PAC signatures.

Class name hashing uses `fnv1a(className, prngSeed)`, which is deterministic per (name, seed) pair regardless of TU or processing order. The shared PRNG can't be used here because it reseeds per module, which would produce name collisions across TUs - causing ObjC runtime conflicts and PAC failures.

## Flags

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_ACD` | off | Master switch (Darwin only) |
| `ACD_PROB=n` | 100 | Per-class probability (0-100). Hash-based (`fnv1a(className, prngSeed)`) - deterministic and monotonic |
| `ACD_CLASS_ALIAS` | off | Register original class names as runtime aliases so `NSClassFromString` / `objc_getClass` still resolve. Creates a transparent empty subclass per processed class |

Runtime overhead is entirely at load time - zero steady-state cost. A class with 20 methods adds ~1-2 microseconds of `+load` time (`sel_registerName` + `class_replaceMethod` per method). Noise compared to dyld's own image loading cost.

## Per-class annotations

```objc
// Exclude a class (annotate any method, instance or class)
__attribute__((annotate("noacd")))
- (void)sensitiveMethod { ... }
```

If any method in a class has `"noacd"`, the entire class is skipped. Note: ObjC method annotations only work reliably at `-O0`. AppleClang at `-O1` does not emit `llvm.global.annotations` entries for ObjC methods - this is a compiler limitation.
