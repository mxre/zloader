/*
 * Copyright (c) Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTD_COMPILER_H
#define ZSTD_COMPILER_H

/*-*******************************************************
*  Compiler specifics
*********************************************************/
/* force inlining */

#if !defined(ZSTD_NO_INLINE)
#if (defined(__GNUC__) && !defined(__STRICT_ANSI__)) || defined(__cplusplus) || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#  define INLINE_KEYWORD inline
#else
#  define INLINE_KEYWORD
#endif

#if defined(__GNUC__) || defined(__ICCARM__)
#  define FORCE_INLINE_ATTR __attribute__((always_inline))
#else
#  define FORCE_INLINE_ATTR
#endif

#else

#define INLINE_KEYWORD
#define FORCE_INLINE_ATTR

#endif

/*
  On MSVC qsort requires that functions passed into it use the __cdecl calling conversion(CC).
  This explictly marks such functions as __cdecl so that the code will still compile
  if a CC other than __cdecl has been made the default.
*/
#define WIN_CDECL

/*
 * FORCE_INLINE_TEMPLATE is used to define C "templates", which take constant
 * parameters. They must be inlined for the compiler to eliminate the constant
 * branches.
 */
#define FORCE_INLINE_TEMPLATE static INLINE_KEYWORD FORCE_INLINE_ATTR
/*
 * HINT_INLINE is used to help the compiler generate better code. It is *not*
 * used for "templates", so it can be tweaked based on the compilers
 * performance.
 *
 * gcc-4.8 and gcc-4.9 have been shown to benefit from leaving off the
 * always_inline attribute.
 *
 * clang up to 5.0.0 (trunk) benefit tremendously from the always_inline
 * attribute.
 */
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 8 && __GNUC__ < 5
#  define HINT_INLINE static INLINE_KEYWORD
#else
#  define HINT_INLINE static INLINE_KEYWORD FORCE_INLINE_ATTR
#endif

/* UNUSED_ATTR tells the compiler it is okay if the function is unused. */
#if defined(__GNUC__)
#  define UNUSED_ATTR __attribute__((unused))
#else
#  define UNUSED_ATTR
#endif

/* force no inlining */
#if defined(__GNUC__) || defined(__ICCARM__)
#  define FORCE_NOINLINE static __attribute__((__noinline__))
#else
#  define FORCE_NOINLINE static
#endif


/* target attribute */
#ifndef __has_attribute
  #define __has_attribute(x) 0  /* Compatibility with non-clang compilers. */
#endif
#if defined(__GNUC__) || defined(__ICCARM__)
#  define TARGET_ATTRIBUTE(target) __attribute__((__target__(target)))
#else
#  define TARGET_ATTRIBUTE(target)
#endif

/* Enable runtime BMI2 dispatch based on the CPU.
 * Enabled for clang & gcc >=4.8 on x86 when BMI2 isn't enabled by default.
 */
#ifndef DYNAMIC_BMI2
  #if ((defined(__clang__) && __has_attribute(__target__)) \
      || (defined(__GNUC__) \
          && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))) \
      && (defined(__x86_64__) || defined(_M_X86)) \
      && !defined(__BMI2__)
  #  define DYNAMIC_BMI2 1
  #else
  #  define DYNAMIC_BMI2 0
  #endif
#endif

/* prefetch
 * can be disabled, by declaring NO_PREFETCH build macro */
#if defined(NO_PREFETCH)
#  define PREFETCH_L1(ptr)  (void)(ptr)  /* disabled */
#  define PREFETCH_L2(ptr)  (void)(ptr)  /* disabled */
#else
#  if defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define PREFETCH_L1(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 3 /* locality */)
#    define PREFETCH_L2(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 2 /* locality */)
#  elif defined(__aarch64__)
#    define PREFETCH_L1(ptr)  __asm__ __volatile__("prfm pldl1keep, %0" ::"Q"(*(ptr)))
#    define PREFETCH_L2(ptr)  __asm__ __volatile__("prfm pldl2keep, %0" ::"Q"(*(ptr)))
#  else
#    define PREFETCH_L1(ptr) (void)(ptr)  /* disabled */
#    define PREFETCH_L2(ptr) (void)(ptr)  /* disabled */
#  endif
#endif  /* NO_PREFETCH */

#define CACHELINE_SIZE 64

#define PREFETCH_AREA(p, s)  {            \
    const char* const _ptr = (const char*)(p);  \
    size_t const _size = (size_t)(s);     \
    size_t _pos;                          \
    for (_pos=0; _pos<_size; _pos+=CACHELINE_SIZE) {  \
        PREFETCH_L2(_ptr + _pos);         \
    }                                     \
}

/* vectorization
 * older GCC (pre gcc-4.3 picked as the cutoff) uses a different syntax */
#if !defined(__INTEL_COMPILER) && !defined(__clang__) && defined(__GNUC__)
#  if (__GNUC__ == 4 && __GNUC_MINOR__ > 3) || (__GNUC__ >= 5)
#    define DONT_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
#  else
#    define DONT_VECTORIZE _Pragma("GCC optimize(\"no-tree-vectorize\")")
#  endif
#else
#  define DONT_VECTORIZE
#endif

/* Tell the compiler that a branch is likely or unlikely.
 * Only use these macros if it causes the compiler to generate better code.
 * If you can remove a LIKELY/UNLIKELY annotation without speed changes in gcc
 * and clang, please do.
 */
#if defined(__GNUC__)
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

/* disable warnings */

/*Like DYNAMIC_BMI2 but for compile time determination of BMI2 support*/


/* compile time determination of SIMD support */
#if !defined(ZSTD_NO_INTRINSICS)
#  if defined(__SSE2__) || defined(_M_AMD64) || (defined (_M_IX86) && defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
#    define ZSTD_ARCH_X86_SSE2
#  endif
#  if defined(__ARM_NEON) || defined(_M_ARM64)
#    define ZSTD_ARCH_ARM_NEON
#  endif
#
#  if defined(ZSTD_ARCH_X86_SSE2)
#    include <emmintrin.h>
#  elif defined(ZSTD_ARCH_ARM_NEON)
#    include <arm_neon.h>
#  endif
#endif  

/* compat. with non-clang compilers */
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

/* compat. with non-clang compilers */
#ifndef __has_feature
#  define __has_feature(x) 0
#endif

#endif /* ZSTD_COMPILER_H */
