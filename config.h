/*
 * config.h - Obscura Configuration Header
 *
 * Include this header to enable explicit control over encryption behavior.
 * All options are configured via compiler flags (-D).
 *
 * See README.md for complete documentation.
 */

#ifndef OBSCURA_CONFIG_H
#define OBSCURA_CONFIG_H

/*============================================================================*
 * ENCRYPTION LEVELS
 *============================================================================*/

/* Header inclusion marker - signals explicit configuration mode */
__attribute__((used)) static int __enc_header_marker = 1;

/* ENC_FULL expands to both ENC_LITE and ENC_DEEP */
#ifdef ENC_FULL
#define ENC_LITE
#define ENC_DEEP
#endif

#ifdef ENC_LITE
__attribute__((used)) static int __enc_lite_marker = 1;
#endif

#ifdef ENC_DEEP
__attribute__((used)) static int __enc_deep_marker = 1;
#endif

/*============================================================================*
 * ENCRYPTION OPTIONS
 *============================================================================*/

/* Iteration count - ENC_FULL_TIMES sets both */
#ifdef ENC_FULL_TIMES
  #ifndef ENC_LITE_TIMES
    #define ENC_LITE_TIMES ENC_FULL_TIMES
  #endif
  #ifndef ENC_DEEP_TIMES
    #define ENC_DEEP_TIMES ENC_FULL_TIMES
  #endif
#endif

#ifndef ENC_LITE_TIMES
#define ENC_LITE_TIMES 1
#endif
#ifndef ENC_DEEP_TIMES
#define ENC_DEEP_TIMES 1
#endif

#if ENC_LITE_TIMES > 1
__attribute__((used)) static int __enc_lite_times = ENC_LITE_TIMES;
#endif
#if ENC_DEEP_TIMES > 1
__attribute__((used)) static int __enc_deep_times = ENC_DEEP_TIMES;
#endif

/* Inline decryption */
#ifdef ENC_DEEP_INLINE
__attribute__((used)) static int __enc_deep_inline = 1;
#endif

/*============================================================================*
 * FILTERS - Variable Selection
 *============================================================================*/

/* Exclude variable from encryption - works for both locals and globals:
 *   NO_ENC int global_var = 1;     // Global: annotation in IR metadata
 *   NO_ENC int local_var = 2;      // Local: generates llvm.var.annotation call
 *
 * For globals: The annotation appears directly in IR metadata, checked by EncryptionFilter.
 * For locals: Clang emits @llvm.var.annotation intrinsic, checked by L2G pass.
 *
 * Usage with L2G:
 *   L2G NO_ENC int secret = 42;    // Promoted to global but NOT encrypted
 */
#define NO_ENC __attribute__((annotate("no_encrypt")))

/*----------------------------------------------------------------------------*
 * Blacklist Filters - Exclude matching variables
 *----------------------------------------------------------------------------*/

/* ENC_SKIP_NAME="pattern" or "pattern1,pattern2,pattern3" */
#ifdef ENC_SKIP_NAME
__attribute__((used)) static const char __enc_skip_name[] = ENC_SKIP_NAME;
#endif

/* ENC_SKIP_BITS="32" or "8,16,32" */
#ifdef ENC_SKIP_BITS
__attribute__((used)) static const char __enc_skip_bits[] = ENC_SKIP_BITS;
#endif

/* ENC_SKIP_FLOATS - exclude all floating-point types */
#ifdef ENC_SKIP_FLOATS
__attribute__((used)) static int __enc_skip_floats = 1;
#endif

/* ENC_SKIP_INTEGERS - exclude all integer types */
#ifdef ENC_SKIP_INTEGERS
__attribute__((used)) static int __enc_skip_integers = 1;
#endif

/*----------------------------------------------------------------------------*
 * Whitelist Filters - Encrypt only matching variables
 *----------------------------------------------------------------------------*/

/* ENC_ONLY_NAME="pattern" or "pattern1,pattern2,pattern3" */
#ifdef ENC_ONLY_NAME
__attribute__((used)) static const char __enc_only_name[] = ENC_ONLY_NAME;
#endif

/* ENC_ONLY_BITS="32" or "8,16,32" */
#ifdef ENC_ONLY_BITS
__attribute__((used)) static const char __enc_only_bits[] = ENC_ONLY_BITS;
#endif

/* ENC_ONLY_FLOATS - encrypt only floating-point types */
#ifdef ENC_ONLY_FLOATS
__attribute__((used)) static int __enc_only_floats = 1;
#endif

/* ENC_ONLY_INTEGERS - encrypt only integer types */
#ifdef ENC_ONLY_INTEGERS
__attribute__((used)) static int __enc_only_integers = 1;
#endif

/*----------------------------------------------------------------------------*
 * Array Filters
 *----------------------------------------------------------------------------*/

/* ENC_SKIP_ARRAYS - skip all array encryption */
#ifdef ENC_SKIP_ARRAYS
__attribute__((used)) static int __enc_skip_arrays = 1;
#endif

/* ENC_ARRAYS_LITE_ONLY - arrays only get lightweight encryption */
#ifdef ENC_ARRAYS_LITE_ONLY
__attribute__((used)) static int __enc_arrays_lite_only = 1;
#endif

/*============================================================================*
 * LOCAL-TO-GLOBAL PROMOTION (L2G)
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Per-Variable Annotations
 *----------------------------------------------------------------------------*/

/* Mark a local variable for L2G promotion: L2G int secret = 42;
 * Can be combined with NO_ENC in any order:
 *   L2G NO_ENC int x = 1;  // promoted but not encrypted
 *   NO_ENC L2G int x = 1;  // same effect
 */
#define L2G __attribute__((annotate("l2g")))

/* Exclude a local variable from automatic L2G promotion: NO_L2G int val = 123;
 * Only relevant when L2G_ENABLE is set (automatic promotion mode).
 */
#define NO_L2G __attribute__((annotate("no_l2g")))

/*----------------------------------------------------------------------------*
 * L2G Configuration (via -D)
 *----------------------------------------------------------------------------*/

/* L2G_ENABLE - enables automatic promotion with all defaults */
#ifdef L2G_ENABLE
__attribute__((used)) static int __l2g_enabled = 1;
#endif

/*----------------------------------------------------------------------------*
 * Type Control (via -D)
 *----------------------------------------------------------------------------*/

/* L2G_INTEGERS=0/1 - control integer promotion */
#ifdef L2G_INTEGERS
__attribute__((used)) static int __l2g_integers = L2G_INTEGERS;
#endif

/* L2G_FLOATS=0/1 - control float promotion */
#ifdef L2G_FLOATS
__attribute__((used)) static int __l2g_floats = L2G_FLOATS;
#endif

/* L2G_INT_ARRAYS=0/1 - control integer array/vector promotion */
#ifdef L2G_INT_ARRAYS
__attribute__((used)) static int __l2g_int_arrays = L2G_INT_ARRAYS;
#endif

/* L2G_FLOAT_ARRAYS=0/1 - control float array/vector promotion */
#ifdef L2G_FLOAT_ARRAYS
__attribute__((used)) static int __l2g_float_arrays = L2G_FLOAT_ARRAYS;
#endif

/* L2G_ALL - enable all type categories */
#ifdef L2G_ALL
__attribute__((used)) static int __l2g_all = 1;
#endif

/*----------------------------------------------------------------------------*
 * L2G Options (via -D)
 *----------------------------------------------------------------------------*/

/* L2G_OPS - enable binary operation result promotion */
#ifdef L2G_OPS
__attribute__((used)) static int __l2g_ops = 1;
#endif

/* L2G_DEDUP - enable constant deduplication */
#ifdef L2G_DEDUP
__attribute__((used)) static int __l2g_dedup = 1;
#endif

/* L2G_PROB=n - probability 0-100 (default: 100) */
#ifdef L2G_PROB
__attribute__((used)) static int __l2g_probability = L2G_PROB;
#endif

/* L2G_MAX_ARRAY=n - max array size (default: 1024, 0=unlimited) */
#ifdef L2G_MAX_ARRAY
__attribute__((used)) static int __l2g_max_array = L2G_MAX_ARRAY;
#endif

#endif /* OBSCURA_CONFIG_H */
