set(SOURCES
    lz4.c
    lz4frame.c
    lz4hc.c
)

add_library(lz4 OBJECT ${SOURCES})
target_compile_options(lz4
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE -std=gnu2x )
