#pragma once

#include "efi/compiler.h"
#include "efilib/config.h"
#include "efilib/print.h"

#define __join(a,b) a ## b
#define _u(t) __join(u, t)

#define xstr(s) str(s)
#define str(s) #s

#ifdef EFILIB_DEBUG
#   define EFILIB_DBG_MESSAGE(msg) print(_u("EFILIB: ") _u(msg) _u("\r\n"))
#   if EFILIB_PRINTF
#      define EFILIB_DBG_PRINTF(msg, ...) wprintf(_u("EFILIB: ") _u(msg) _u("\n") __VA_OPT__(, __VA_ARGS__))
#   else
#      define EFILIB_DBG_PRINTF(...) if (!(condition)) { \
        [[ clang::unlikely ]] \
        print(_u("ASSERT " __FILE__ ":" xstr(__LINE__) " " #condition "\r\n")); \
        exit(EFI_UNSUPPORTED); }
#   endif 
#   define EFILIB_ASSERT(condition) 
#else
#  define EFILIB_DBG_MESSAGE(msg)
#  define EFILIB_DBG_PRINTF(...)
#  define EFILIB_ASSERT(condition)
#endif /* EFI_DEBUG */

#define EFILIB_ERROR(msg) print(_u("EFILIB: ERROR: ") _u(msg) _u("\r\n"))

#define __efi_assert(condition) if (!(condition)) { \
        [[ clang::unlikely ]] \
        print(_u("ASSERT " __FILE__ ":" xstr(__LINE__) " " #condition "\r\n")); \
        exit(EFI_UNSUPPORTED); }
