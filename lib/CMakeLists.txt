set(SOURCES
    xxhash.c
)

add_library(lib OBJECT ${SOURCES})
target_compile_options(lib
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE -std=gnu2x )
