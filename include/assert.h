#pragma once

#include "efilib/debug.h"

#if !__has_feature(c_static_assert)
#define static_assert _Static_assert
#endif

#ifdef NDEBUG
# define assert(condition) (void)0
#else
# define assert(condition) __efi_assert(condition)
#endif
