#pragma once

#ifndef PAGE_SIZE /* can be overriden by compiler command line */
#  define PAGE_SIZE 0x1000 /* default value for all supported platforms */
#endif

#define PROGRAM_NAME "zloader"
#define PROGRAM_VERSION "0.1"
