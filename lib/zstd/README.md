ZSTD Source code extracted from https://github.com/facebook/zstd commit `v1.4.7-479-gb3e372c1` with the distributed freestanding library tool using the following options:

```
../freestanding_lib/freestanding.py \
    --source-lib ../../lib \
    --output-lib lib/zstd \
    --xxhash '<xxhash.h>' \
    --xxh64-state 'struct xxh64_state' \
    --xxh64-prefix 'xxh64' \
    --rewrite-include '"\.\./zstd.h"=<zstd.h>' \
    --rewrite-include '"(\.\./)?zstd_errors.h"=<zstd_errors.h>' \
    --sed 's,/\*\*\*,/* *,g' \
    --sed 's,/\*\*,/*,g' \
    -DZSTD_NO_UNUSED_FUNCTIONS \
    -DZSTD_LEGACY_SUPPORT=0 \
    -DMEM_FORCE_MEMORY_ACCESS=1 \
    -DSTATIC_BMI2=0 \
    -DZSTD_ADDRESS_SANITIZER=0 \
    -DZSTD_MEMORY_SANITIZER=0 \
    -DZSTD_COMPRESS_HEAPMODE=1 \
    -UZSTD_DLL_EXPORT \
    -UZSTD_DLL_IMPORT \
    -U__ICCARM__ \
    -UZSTD_MULTITHREAD \
    -U_MSC_VER \
    -U_WIN32 \
    -RZSTDLIB_VISIBILITY= \
    -RZSTDERRORLIB_VISIBILITY= \
    -DZSTD_HAVE_WEAK_SYMBOLS=0 \
    -DZSTD_TRACE=0 \
    -DZSTD_NO_TRACE
```
