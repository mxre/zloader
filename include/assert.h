#pragma once

#include <efilib.h>

#if !__has_feature(c_static_assert)
#define static_assert _Static_assert
#endif

#ifdef NDEBUG
# define assert(condition)
#else
# define assert(condition) EFILIB_ASSERT(condition)
#endif
