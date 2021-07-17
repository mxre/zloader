#pragma once

#include "efilib/config.h"
#include "efilib/print.h"

#define __join(a,b) a ## b
#define _u(t) __join(u, t)

#ifdef EFI_DEBUG
#  define EFI_DBG_MESSAGE(msg) print(_u("EFILIB: ") _u(msg) _u("\r\n"))
#  if EFI_PRINTF
#    define EFI_DBG_PRINTF(msg, ...) printf(_u("EFILIB: ") _u(msg) _u("\n") __VA_OPT__(, __VA_ARGS__))
#  else
#    define EFI_DBG_PRINTF(...) 
#  endif 
#  define ASSERT(condition)
#else
#  define EFI_DBG_MESSAGE(msg)
#  define EFI_DBG_PRINTF(...)
#  define ASSERT(condition)
#endif /* EFI_DEBUG */
