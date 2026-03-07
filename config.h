/*
 * config.h — Obscura configuration
 *
 * Include this (somehow) and pass -D flags to control what gets obfuscated.
 * Full docs can be found in the original repository at https://github.com/nkhmelni/Obscura.
 */

#ifndef OBSCURA_CONFIG_H
#define OBSCURA_CONFIG_H

/* Sentinel: tells the plugin that config.h is included.
   Without this, the plugin applies built-in defaults. */
__attribute__((used)) static int __obscura_config = 1;

/* Hikari-like flag aliases — resolved before anything else.
 * Most of these have, in fact, very little to do with Hikari. */

#ifdef ENABLE_FLA
  #ifndef ENABLE_CFF
    #define ENABLE_CFF
  #endif
#endif

#ifdef ENABLE_STRENC
  #ifndef ENABLE_STRCRY
    #define ENABLE_STRCRY
  #endif
#endif

#ifdef ENABLE_FW
  #ifndef ENABLE_FUNCWRA
    #define ENABLE_FUNCWRA
  #endif
#endif

#ifdef ENABLE_INDIBR
  #ifndef ENABLE_INDIBRAN
    #define ENABLE_INDIBRAN
  #endif
#endif

#ifdef ENABLE_BCFOBF
  #ifndef ENABLE_BCF
    #define ENABLE_BCF
  #endif
#endif

#ifdef ENABLE_CFFOBF
  #ifndef ENABLE_CFF
    #define ENABLE_CFF
  #endif
#endif

#ifdef ENABLE_SUBOBF
  #ifndef ENABLE_SUB
    #define ENABLE_SUB
  #endif
#endif

#ifdef ENABLE_SPLITOBF
  #ifndef ENABLE_SPLIT
    #define ENABLE_SPLIT
  #endif
#endif

#ifdef ENABLE_ACDOBF
  #ifndef ENABLE_ACD
    #define ENABLE_ACD
  #endif
#endif

/*--- Constant Encryption levels ---*/

/* CONSTENC_FULL = both LITE and DEEP */
#ifdef CONSTENC_FULL
#define CONSTENC_LITE
#define CONSTENC_DEEP
#endif

#ifdef CONSTENC_LITE
__attribute__((used)) static int __constenc_lite_marker = 1;
#endif

#ifdef CONSTENC_DEEP
__attribute__((used)) static int __constenc_deep_marker = 1;
#endif

/*--- Constant Encryption options ---*/

/* CONSTENC_FULL_TIMES sets both lite and deep iteration counts */
#ifdef CONSTENC_FULL_TIMES
  #ifndef CONSTENC_LITE_TIMES
    #define CONSTENC_LITE_TIMES CONSTENC_FULL_TIMES
  #endif
  #ifndef CONSTENC_DEEP_TIMES
    #define CONSTENC_DEEP_TIMES CONSTENC_FULL_TIMES
  #endif
#endif

#ifndef CONSTENC_LITE_TIMES
#define CONSTENC_LITE_TIMES 1
#endif
#ifndef CONSTENC_DEEP_TIMES
#define CONSTENC_DEEP_TIMES 1
#endif

#if CONSTENC_LITE_TIMES > 1
__attribute__((used)) static int __constenc_lite_times = CONSTENC_LITE_TIMES;
#endif
#if CONSTENC_DEEP_TIMES > 1
__attribute__((used)) static int __constenc_deep_times = CONSTENC_DEEP_TIMES;
#endif

#ifdef CONSTENC_DEEP_INLINE
__attribute__((used)) static int __constenc_deep_inline = 1;
#endif

/* CONSTENC_PROB=n — per-global probability, 0-100 (default 50) */
#ifdef CONSTENC_PROB
__attribute__((used)) static int __constenc_prob = CONSTENC_PROB;
#endif

/*--- Filters — control which variables get encrypted ---*/

/* NO_CONSTENC — exclude a variable from encryption.
 * Works on both globals and locals (with L2G). */
#define NO_CONSTENC __attribute__((annotate("no_constenc")))

/* Blacklist: skip matching variables */

#ifdef CONSTENC_SKIP_NAME
__attribute__((used)) static const char __constenc_skip_name[] = CONSTENC_SKIP_NAME;
#endif

#ifdef CONSTENC_SKIP_BITS
__attribute__((used)) static const char __constenc_skip_bits[] = CONSTENC_SKIP_BITS;
#endif

#ifdef CONSTENC_SKIP_FLOATS
__attribute__((used)) static int __constenc_skip_floats = 1;
#endif

#ifdef CONSTENC_SKIP_INTEGERS
__attribute__((used)) static int __constenc_skip_integers = 1;
#endif

/* Whitelist: encrypt only matching variables */

#ifdef CONSTENC_ONLY_NAME
__attribute__((used)) static const char __constenc_only_name[] = CONSTENC_ONLY_NAME;
#endif

#ifdef CONSTENC_ONLY_BITS
__attribute__((used)) static const char __constenc_only_bits[] = CONSTENC_ONLY_BITS;
#endif

#ifdef CONSTENC_ONLY_FLOATS
__attribute__((used)) static int __constenc_only_floats = 1;
#endif

#ifdef CONSTENC_ONLY_INTEGERS
__attribute__((used)) static int __constenc_only_integers = 1;
#endif

/* Array-specific filters */

#ifdef CONSTENC_SKIP_ARRAYS
__attribute__((used)) static int __constenc_skip_arrays = 1;
#endif

#ifdef CONSTENC_ARRAYS_LITE_ONLY
__attribute__((used)) static int __constenc_arrays_lite_only = 1;
#endif

/*--- Local-to-Global promotion (L2G) ---*/

/* Per-variable annotations */
#define L2G __attribute__((annotate("l2g")))
#define NO_L2G __attribute__((annotate("no_l2g")))

/* L2G_ENABLE — turn on automatic promotion */
#ifdef L2G_ENABLE
__attribute__((used)) static int __l2g_enabled = 1;
#endif

/* Type control (0 or 1) */

#ifdef L2G_INTEGERS
__attribute__((used)) static int __l2g_integers = L2G_INTEGERS;
#endif

#ifdef L2G_FLOATS
__attribute__((used)) static int __l2g_floats = L2G_FLOATS;
#endif

#ifdef L2G_INT_ARRAYS
__attribute__((used)) static int __l2g_int_arrays = L2G_INT_ARRAYS;
#endif

#ifdef L2G_FLOAT_ARRAYS
__attribute__((used)) static int __l2g_float_arrays = L2G_FLOAT_ARRAYS;
#endif

/* L2G options */

#ifdef L2G_OPS
__attribute__((used)) static int __l2g_ops = 1;
#endif

#ifdef L2G_DEDUP
__attribute__((used)) static int __l2g_dedup = 1;
#endif

/* L2G_PROB=n — probability, 0-100 (default 100) */
#ifdef L2G_PROB
__attribute__((used)) static int __l2g_probability = L2G_PROB;
#endif

/* L2G_MAX_ARRAY=n — max array size (default 1024, 0 = unlimited) */
#ifdef L2G_MAX_ARRAY
__attribute__((used)) static int __l2g_max_array = L2G_MAX_ARRAY;
#endif

/*--- Obfuscation passes ---*/

/* Bogus Control Flow */

#ifdef ENABLE_BCF
__attribute__((used)) static int __obf_bcf_marker = 1;
#endif

#ifdef BCF_PROB
__attribute__((used)) static int __obf_bcf_prob = BCF_PROB;          /* 0-100, default 70 */
#endif

#ifdef BCF_LOOP
__attribute__((used)) static int __obf_bcf_loop = BCF_LOOP;          /* iterations, default 1 */
#endif

#ifdef BCF_COND_COMPL
__attribute__((used)) static int __obf_bcf_cond_compl = BCF_COND_COMPL; /* predicate complexity, default 3 */
#endif

#ifdef BCF_JUNKASM
__attribute__((used)) static int __obf_bcf_junkasm = 1;              /* junk asm in unreachable blocks */
#endif

#ifdef BCF_ONLYJUNKASM
__attribute__((used)) static int __obf_bcf_onlyjunkasm = 1;          /* junk asm only, no cloned BB */
#endif

#ifdef BCF_CREATEFUNC
__attribute__((used)) static int __obf_bcf_createfunc = 1;           /* junk functions instead of inline asm */
#endif

#ifdef BCF_JUNKASM_MINNUM
__attribute__((used)) static int __obf_bcf_junkasm_minnum = BCF_JUNKASM_MINNUM; /* default 2 */
#endif

#ifdef BCF_JUNKASM_MAXNUM
__attribute__((used)) static int __obf_bcf_junkasm_maxnum = BCF_JUNKASM_MAXNUM; /* default 4 */
#endif

/* Control Flow Flattening */

#ifdef ENABLE_CFF
__attribute__((used)) static int __obf_cff_marker = 1;
#endif

#ifdef CFF_PROB
__attribute__((used)) static int __obf_cff_prob = CFF_PROB;          /* 0-100, default 100 */
#endif

/* Instruction Substitution */

#ifdef ENABLE_SUB
__attribute__((used)) static int __obf_sub_marker = 1;
#endif

#ifdef SUB_LOOP
__attribute__((used)) static int __obf_sub_loop = SUB_LOOP;          /* iterations, default 1 */
#endif

#ifdef SUB_PROB
__attribute__((used)) static int __obf_sub_prob = SUB_PROB;          /* 0-100, default 50 */
#endif

/* Basic Block Splitting */

#ifdef ENABLE_SPLIT
__attribute__((used)) static int __obf_split_marker = 1;
#endif

#ifdef SPLIT_NUM
__attribute__((used)) static int __obf_split_num = SPLIT_NUM;        /* splits per block, default 2 */
#endif

/* String Encryption */

#ifdef ENABLE_STRCRY
__attribute__((used)) static int __obf_strcry_marker = 1;
#endif

#ifdef STRCRY_PROB
__attribute__((used)) static int __obf_strcry_prob = STRCRY_PROB;    /* 0-100, default 100 */
#endif

/* Function Wrapper */

#ifdef ENABLE_FUNCWRA
__attribute__((used)) static int __obf_funcwra_marker = 1;
#endif

#ifdef FW_PROB
__attribute__((used)) static int __obf_funcwra_prob = FW_PROB;       /* 0-100, default 30 */
#endif

#ifdef FW_TIMES
__attribute__((used)) static int __obf_funcwra_times = FW_TIMES;     /* nesting depth, default 2 */
#endif

/* Indirect Branch */

#ifdef ENABLE_INDIBRAN
__attribute__((used)) static int __obf_indibran_marker = 1;
#endif

#ifdef INDIBRAN_PROB
__attribute__((used)) static int __obf_indibran_prob = INDIBRAN_PROB; /* 0-100, default 100 */
#endif

/* Function Call Obfuscate (Darwin — replaces calls with dlsym lookups) */

#ifdef ENABLE_FCO
__attribute__((used)) static int __obf_fco_marker = 1;
#endif

#ifdef FCO_HIDE_FW
  #ifndef ENABLE_FCO
    #define ENABLE_FCO
    __attribute__((used)) static int __obf_fco_marker = 1;
  #endif
  __attribute__((used)) static int __obf_fco_hide_fw = 1;
#endif

#ifdef FCO_FLAG
__attribute__((used)) static int __obf_fco_flag = FCO_FLAG;
#endif

#ifdef FCO_CONFIG
__attribute__((used)) static const char __obf_fco_config[] = FCO_CONFIG;
#endif

/* Anti-Debug (AArch64 Darwin — ptrace checks) */

#ifdef ENABLE_ADB
__attribute__((used)) static int __obf_adb_marker = 1;
#endif

#ifdef ADB_PROB
__attribute__((used)) static int __obf_adb_prob = ADB_PROB;          /* 0-100, default 40 */
#endif

/* Anti-ClassDump (ObjC Darwin — hides class metadata) */

#ifdef ENABLE_ACD
__attribute__((used)) static int __obf_acd_marker = 1;
#endif

#ifdef ACD_PROB
__attribute__((used)) static int __obf_acd_prob = ACD_PROB;          /* 0-100, default 100 */
#endif

#ifdef ACD_CLASS_ALIAS
__attribute__((used)) static int __obf_acd_class_alias = 1;
#endif

/*--- Global options ---*/

/* PRNG_SEED=n — fixed seed for reproducible output (default: time-based) */
#ifdef PRNG_SEED
__attribute__((used)) static long long __obf_prng_seed = PRNG_SEED;
#endif

/*--- Hikari compatibility reference ---*
 *
 * Flag aliases (resolved at top of file):
 *   ENABLE_FLA      -> ENABLE_CFF       ENABLE_STRENC   -> ENABLE_STRCRY
 *   ENABLE_FW       -> ENABLE_FUNCWRA   ENABLE_INDIBR   -> ENABLE_INDIBRAN
 *   ENABLE_BCFOBF   -> ENABLE_BCF       ENABLE_CFFOBF   -> ENABLE_CFF
 *   ENABLE_SUBOBF   -> ENABLE_SUB       ENABLE_SPLITOBF -> ENABLE_SPLIT
 *   ENABLE_ACDOBF   -> ENABLE_ACD
 *
 * Per-function annotation aliases (handled by plugin):
 *   fla/nofla             -> cff/nocff
 *   strenc/nostrenc       -> strcry/nostrcry
 *   fw/nofw               -> funcwra/nofuncwra
 *   fw_prob=N/fw_times=N  -> funcwra_prob=N/funcwra_times=N
 *   indibr/noindibr       -> indibran/noindibran
 *   cffobf/nocffobf       -> cff/nocff
 */

/* Per-function annotation helper */
#define OBSCURA_ANNOTATE(str) __attribute__((__annotate__(str)))

#endif /* OBSCURA_CONFIG_H */
