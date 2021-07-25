project(zloader C)
cmake_minimum_required(VERSION 3.21)

add_subdirectory(tools)
include_directories(SYSTEM "include/")

set(LOADER_TARGET "x86_64" CACHE STRING "Target Processor Architecture")
set_property(CACHE LOADER_TARGET PROPERTY STRINGS aarch64 x86_64)
option(LOADER_USE_LZ4 "Enable LZ4 decompression" ON)
option(LOADER_USE_ZSTD "Enable ZSTD decompression" OFF)
option(LOADER_PRINT_MESSAGES "Print non-error messages to console" OFF)
option(LOADER_USE_EFI_LOAD_IMAGE "Use LoadImage and StartImage instead of own parser (needs embedded Image to be signed for SecureBoot)" OFF)

set(CMAKE_C_COMPILER clang)
set(CMAKE_LINKER lld-link)
set(CMAKE_RC_COMPILER llvm-rc)

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES> -out:<TARGET> ")
set(CMAKE_EXECUTABLE_SUFFIX ".efi")
set(COMPILE_TARGET "${LOADER_TARGET}-none-windows")
add_compile_options(-mno-stack-arg-probe -fno-stack-protector -ffreestanding -mno-red-zone)

if(LOADER_TARGET STREQUAL "x86_64")
  set(EFI_ARCH "x64")
elseif(LOADER_TARGET STREQAL "aarch64")
  set(EFI_ARCH "aa64")
else()
  message(FATAL_ERROR "Architecture ${LOADER_TARGET} is unsupported")
endif()

add_subdirectory(lib)
add_subdirectory(lib/efilib)

if(LOADER_USE_LZ4)
  add_subdirectory(lib/lz4)
  list(APPEND OPTIONAL_DEPENDENCIES $<TARGET_OBJECTS:lz4>)
  add_compile_definitions(USE_LZ4)
endif(LOADER_USE_LZ4)

if(LOADER_USE_ZSTD)
  add_subdirectory(lib/zstd)
  list(APPEND OPTIONAL_DEPENDENCIES $<TARGET_OBJECTS:zstd>)
  add_compile_definitions(USE_ZSTD)
endif(LOADER_USE_ZSTD)

if(LOADER_PRINT_MESSAGES)
  add_compile_definitions(PRINT_MESSAGES)
endif(LOADER_PRINT_MESSAGES)

if(LOADER_USE_EFI_LOAD_IMAGE)
  add_compile_definitions(USE_EFI_LOAD_IMAGE)
endif(LOADER_USE_EFI_LOAD_IMAGE)
add_subdirectory(src)

add_executable(zloader $<TARGET_OBJECTS:src> $<TARGET_OBJECTS:efilib> $<TARGET_OBJECTS:lib> ${OPTIONAL_DEPENDENCIES})
set_target_properties(zloader PROPERTIES
  SUFFIX "${EFI_ARCH}.efi.stub"
  LINK_FLAGS "-subsystem:efi_application -tsaware:no -entry:efi_main" )
