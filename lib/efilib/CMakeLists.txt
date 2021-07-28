set(SOURCES
    efilib.c
    efirtlib.c
    efifprt.c
    efiprint.c
    efidp.c
    efivar.c
    guid.c
    string.c
)

option(EFILIB_SHUTDOWN "Issue PowerOff on program exit (usefull for emulators)" OFF)
option(EFILIB_DEBUG "Write debug messages and use asserts in library" OFF)
option(EFILIB_USE_EFI_SET_MEM "Use BootServices SetMem instead of own implementation" OFF)
option(EFILIB_USE_EFI_COPY_MEM "Use BootServices CopyMem instead of own implementation" OFF)
option(EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL "Use DevicePathToTextProtocol to print DevicePaths" ON)
set(EFILIB_STALL_ON_EXIT 5000000 CACHE STRING "Stall for microseconds before exit (in order to read messages)")

if(EFILIB_SHUTDOWN)
  add_compile_definitions(EFILIB_SHUTDOWN)
endif(EFILIB_SHUTDOWN)

if(EFILIB_DEBUG)
  add_compile_definitions(EFILIB_DEBUG)
endif(EFILIB_DEBUG)

if(EFILIB_USE_EFI_COPY_MEM)
  add_compile_definitions(EFILIB_USE_EFI_COPY_MEM)
endif(EFILIB_USE_EFI_COPY_MEM)

if(EFILIB_USE_EFI_SET_MEM)
  add_compile_definitions(EFILIB_USE_EFI_SET_MEM)
endif(EFILIB_USE_EFI_SET_MEM)

if(EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL)
  add_compile_definitions(EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL)
endif(EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL)

if(EFILIB_STALL_ON_EXIT GREATER 10000)
  add_compile_definitions(EFILIB_STALL_ON_EXIT=${EFILIB_STALL_ON_EXIT})
endif()

add_library(efilib OBJECT ${SOURCES})
target_compile_options(efilib 
  PUBLIC -target ${COMPILE_TARGET}
  PRIVATE -std=gnu2x
)

set_source_files_properties(efifprt.c PROPERTIES
  COMPILE_OPTIONS "-fno-lto"
)
