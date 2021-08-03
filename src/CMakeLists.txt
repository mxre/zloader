set(SOURCES
    main.c
    util.c
    systemd.c
    pe.c
    initrd.c
    decompress.c
)

if(LOADER_TARGET STREQUAL "aarch64")
  # useless on x86 but does not harm if included on aarch64
  list(APPEND SOURCES fdt_fixup.c)
  add_compile_definitions(USE_EFI_DT_FIXUP)
endif(LOADER_TARGET STREQUAL "aarch64")

if(LOADER_USE_EFI_LOAD_IMAGE)
  add_compile_definitions(USE_EFI_LOAD_IMAGE)
else()
  list(APPEND SOURCES pe_loader.c)
endif(LOADER_USE_EFI_LOAD_IMAGE)

add_library(src OBJECT ${SOURCES})
target_compile_options(src
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE "-std=gnu2x" "-Wno-gnu-zero-variadic-macro-arguments" "-Wno-gnu-variable-sized-type-not-at-end"
)
