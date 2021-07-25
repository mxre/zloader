set(SOURCES
    main.c
    util.c
    systemd.c
    pe.c
    pe_loader.c
    initrd.c
    decompress.c
  
    main.rc
)

add_library(src OBJECT ${SOURCES})
target_compile_options(src
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE "-std=gnu2x" "-Wno-gnu-zero-variadic-macro-arguments" "-Wno-gnu-variable-sized-type-not-at-end" )
