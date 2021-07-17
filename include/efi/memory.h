#pragma once

#include "defs.h"

enum efi_allocate_t {
	EFI_ALLOCATE_ANY_PAGES,
	EFI_ALLOCATE_MAX_ADDRESS,
	EFI_ALLOCATE_ADDRESS,
	_EFI_ALLOCATE_T_MAX,
};

typedef enum efi_allocate_t efi_allocate_t;

enum efi_memory_t {
	EFI_RESERVED_MEMORY_TYPE,
	EFI_LOADER_CODE,
	EFI_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA,
	EFI_RUNTIME_SERVICES_CODE,
	EFI_RUNTIME_SERVICES_DATA,
	EFI_CONVENTIONAL_MEMORY,
	EFI_UNUSABLE_MEMORY,
	EFI_ACPI_RECLAIM_MEMORY,
	EFI_ACPI_MEMORY_NVS,
	EFI_MEMORY_MAPPED_IO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	EFI_PAL_CODE,
	EFI_PERSISTENT_MEMORY,
	_EFI_MEMORY_T_MAX,
};

typedef enum efi_memory_t efi_memory_t;

typedef uintptr_t efi_physical_address_t;
typedef uintptr_t efi_virtual_address_t;

struct efi_memory_descriptor {
	uint32_t type;
	efi_physical_address_t physical_start;
	efi_virtual_address_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};

typedef struct efi_memory_descriptor* efi_memory_descriptor_t;
