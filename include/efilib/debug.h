#pragma once

#include "efi/compiler.h"
#include "config.h"
#include "print.h"
#include "misc.h"

#define __join(a,b) a ## b
#define _u(t) __join(u, t)

#define xstr(s) str(s)
#define str(s) #s

#ifdef EFILIB_DEBUG
#   define EFILIB_DBG_MESSAGE(msg) print(_u("EFILIB: ") _u(msg) _u("\r\n"))
#   if EFILIB_PRINTF
#      define EFILIB_DBG_PRINTF(msg, ...) wprintf(_u("EFILIB: ") _u(msg) _u("\n") __VA_OPT__(, __VA_ARGS__))
#   else
#      define EFILIB_DBG_PRINTF(msg, ...) EFILIB_DBG_MESSAGE(msg _u" (printf not compiled in)")
#   endif 
#   define EFILIB_ASSERT(condition) if (!(condition)) { \
        __unlikely__ \
        print(_u("ASSERT " __FILE__ ":" xstr(__LINE__) " " #condition "\r\n")); \
        exit(EFI_UNSUPPORTED); \
        __unreachable__; }
#else
#  define EFILIB_DBG_MESSAGE(msg) (void)0
#  define EFILIB_DBG_PRINTF(...) (void)0
#  define EFILIB_ASSERT(condition) (void)0
#endif /* EFI_DEBUG */

#define EFILIB_ERROR(msg) print(_u("EFILIB: ERROR: ") _u(msg) _u("\r\n"))
#if EFILIB_PRINTF
#define EFILIB_ERROR_PRINTF(msg, ...) wprintf(_u("EFILIB: ERROR: ") _u(msg) _u("\n") __VA_OPT__(, __VA_ARGS__))
#else
#define EFILIB_ERROR_PRINTF(msg, ...) EFILIB_ERROR(msg _u" (printf not compiled in)")
#endif

#define __efi_assert(condition) if (!(condition)) { \
        __unlikely__ \
        print(_u("ASSERT " __FILE__ ":" xstr(__LINE__) " " #condition "\r\n")); \
        exit(EFI_UNSUPPORTED); \
        __unreachable__; }
