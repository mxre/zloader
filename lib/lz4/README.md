LZ4 from [https://github.com/lz4/lz4] commit `v1.9.2-334-gc240126` with
changes made to accommodate clang with PE/Windows target uses for compiling
EFI executables.

Specifically removed several `#ifdef` for `_MSVC` since those definitions
tended to confuse clang (which will happiliy use GNU GCC's version instead).
If someone would need to compile for MSVC these definitions have to be
reinitroduced (preferably in the `#else` branch after the GCC definitions)
