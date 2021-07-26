set(SOURCES
    main.c
    util.c
    systemd.c
    pe.c
    pe_loader.c
    initrd.c
    decompress.c
)

if(LOADER_PRINT_MESSAGES)
  add_compile_definitions(PRINT_MESSAGES)
endif(LOADER_PRINT_MESSAGES)

if(LOADER_USE_EFI_LOAD_IMAGE)
  add_compile_definitions(USE_EFI_LOAD_IMAGE)
endif(LOADER_USE_EFI_LOAD_IMAGE)

add_library(src OBJECT ${SOURCES})
target_compile_options(src
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE "-std=gnu2x" "-Wno-gnu-zero-variadic-macro-arguments" "-Wno-gnu-variable-sized-type-not-at-end" )
