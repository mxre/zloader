#pragma once

#include <efilib.h>

#if !__has_feature(c_static_assert)
#define static_assert _Static_assert
#endif

#ifndef NDEBUG
# define assert(condition)
#else
# define assert(condition) EFI_ASSERT(condition)
#endif
