#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "compiler.h"

typedef size_t efi_status_t;
typedef size_t efi_size_t;
typedef void* efi_handle_t;

typedef char char8_t;
typedef uint16_t char16_t;

struct __packed efi_guid {
	union {
		struct {
			uint32_t ms1;
			uint16_t ms2;
			uint16_t ms3;
			uint8_t ms4[8];
		};
		uint8_t u8[16];
		uint16_t u16[8];
		uint32_t u32[4];
		uint64_t u64[2];
	};
};

typedef struct efi_guid* efi_guid_t;

struct efi_table_header {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
};

#if __x86_64__
#  define efi_api __attribute__((ms_abi))
#elif __i386__ || __i686__
#  define efi_api __attribute__((cdecl))
#else
#  define efi_api
#endif

/* well known UEFI Machine Abbreviations */
#if __x86_64__
#  define EFI_ARCH "x64"
#elif __i386__ || __i686__
#  define EFI_ARCH "ia32"
#elif __aarch64__
#  define EFI_ARCH "aa64"
#elif __arm__
#  define EFI_ARCH "arm"
#elif __riscv && __riscv_xlen == 64
#  define EFI_ARCH "riscv64"
#elif __riscv && __riscv_xlen == 32
#  define EFI_ARCH "riscv32"
#else
#  error "Unsupported UEFI System"
#endif

#define LOAD_OPTION_ACTIVE          UINT32_C(0x00000001)
#define LOAD_OPTION_FORCE_RECONNECT UINT32_C(0x00000002)
#define LOAD_OPTION_HIDDEN          UINT32_C(0x00000008)
#define LOAD_OPTION_CATEGORY        UINT32_C(0x00001F00)
#define LOAD_OPTION_CATEGORY_BOOT   UINT32_C(0x00000000)
#define LOAD_OPTION_CATEGORY_APP    UINT32_C(0x00000100)

struct __packed efi_load_option {
    uint32_t attributes;
    uint16_t file_path_list_length;
    uint16_t description[];
//    efi_filepath_t file_path_list[];
//    uint8_t optional_data[]
};

typedef struct efi_load_option* efi_load_option_t;
