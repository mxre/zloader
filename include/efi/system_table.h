#pragma once

#include "defs.h"
#include "simple_text_output_protocol.h"
#include "runtime_services.h"
#include "boot_services.h"

#define EFI_SYSTEM_TABLE_SIGNATURE UINT64_C(0x5453595320494249) /* "IBI SYST" */

struct efi_system_table {
	struct efi_table_header header;
	char16_t* firmware_vendor;
	uint32_t firmware_revision;
	efi_handle_t console_in_handle;
	void* in;
	efi_handle_t console_out_handle;
	efi_simple_text_output_protocol_t out;
	efi_handle_t standard_error_handle;
	efi_simple_text_output_protocol_t err;
	efi_runtime_services_table_t runtime_services;
	efi_boot_services_table_t boot_services;
	efi_size_t number_of_table_entries;
	void* configuration_table;
};

typedef struct efi_system_table* efi_system_table_t;
