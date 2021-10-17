#pragma once

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#ifndef __has_feature
#  define __has_feature(x) 0
#endif

#ifndef __is_identifier
#  define __is_identifier(x) 0
#endif

#ifndef __has_c_attribute
#  define __has_c_attribute(x) 0
#endif

#ifndef __always_inline
#  if __has_attribute(always_inline)
#    define __always_inline __attribute((always_inline))
#  else
#    define __always_inline
#  endif
#endif

#ifndef __packed
#  if __has_c_attribute(gnu::packed)
#    define __packed [[ gnu::packed ]]
#  else
#    define __packed __attribute((packed))
#  endif
#endif

#ifndef __pure
#  if __has_c_attribute(gnu::pure)
#    define __pure [[ gnu::pure ]]
#  else
#    define __pure
#  endif
#endif

#ifndef __constexp
#  if __has_c_attribute(gnu::const)
#    define __constexp [[ gnu::const ]]
#  else
#    define __constexp
#  endif
#endif

#ifndef __unlikely__
#  if __has_c_attribute(unlikely)
#    define __unlikely__ [[ unlikely ]]
#  else
#    define __unlikely__
#  endif
#endif

#ifndef __fallthrough__
#  if __has_c_attribute(fallthough)
#    define __fallthrough__ [[ fallthough ]]
#  else
#    define __fallthrough__
#  endif
#endif

#ifndef __weak__
#  if __has_c_attribute(gnu::weak)
#    define __weak__ [[ gnu::weak ]]
#  else
#    define __weak__
#  endif
#endif

#include <stdnoreturn.h>

#ifndef __unreachable__
#  if __has_builtin(__builtin_unreachable)
#    define __unreachable__ __builtin_unreachable()
#  else
#    define __unreachable__
#  endif
#endif
