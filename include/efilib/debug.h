#pragma once

#include "efilib/config.h"
#include "efilib/print.h"

#define __join(a,b) a ## b
#define _u(t) __join(u, t)

#ifdef EFILIB_DEBUG
#  define EFILIB_DBG_MESSAGE(msg) print(_u("EFILIB: ") _u(msg) _u("\r\n"))
#  if EFILIB_PRINTF
#    define EFILIB_DBG_PRINTF(msg, ...) wprintf(_u("EFILIB: ") _u(msg) _u("\n") __VA_OPT__(, __VA_ARGS__))
#  else
#    define EFILIB_DBG_PRINTF(...) 
#  endif 
#  define EFILIB_ASSERT(condition)
#else
#  define EFILIB_DBG_MESSAGE(msg)
#  define EFILIB_DBG_PRINTF(...)
#  define EFILIB_ASSERT(condition)
#endif /* EFI_DEBUG */
