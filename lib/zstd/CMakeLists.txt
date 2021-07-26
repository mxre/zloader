set(SOURCES
    common/zstd_common.c
    common/entropy_common.c
    common/fse_decompress.c
    common/error_private.c
    decompress/huf_decompress.c
    decompress/zstd_ddict.c
    decompress/zstd_decompress.c
    decompress/zstd_decompress_block.c
)

add_library(zstd OBJECT ${SOURCES})
target_compile_options(zstd
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE -std=gnu2x
)
