#pragma once

#ifndef PAGE_SIZE /* can be overriden by compiler command line */
#  define PAGE_SIZE 0x1000 /* default value for all supported platforms */
#endif

#define LOADER_NAME "zloader"

#ifndef LOADER_VERSION
#  define LOADER_VERSION "0.1"
#endif
