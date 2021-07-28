set(SOURCES
    main.c
    util.c
    systemd.c
    pe.c
    initrd.c
    decompress.c
)

if(LOADER_PRINT_MESSAGES)
  add_compile_definitions(PRINT_MESSAGES)
endif(LOADER_PRINT_MESSAGES)

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

add_executable(fit boot.c systemd.c $<TARGET_OBJECTS:efilib>)
set_target_properties(fit PROPERTIES
  SUFFIX "${EFI_ARCH}.efi"
  LINK_FLAGS "-subsystem:efi_application -tsaware:no -entry:efi_main"
)
target_compile_options(fit
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE "-std=gnu2x" "-Wno-gnu-zero-variadic-macro-arguments" "-Wno-gnu-variable-sized-type-not-at-end"
)
add_custom_command(TARGET fit POST_BUILD
  COMMAND ${CMAKE_BINARY_DIR}/tools/vma_fixer $<TARGET_FILE:fit>
  DEPENDS vma_fixer
)
