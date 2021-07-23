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
