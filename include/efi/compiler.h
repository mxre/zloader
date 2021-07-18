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

#ifndef __always_inline
#  define __always_inline __attribute__((always_inline))
#endif

#ifndef packed
#  define __packed __attribute__((packed))
#endif
