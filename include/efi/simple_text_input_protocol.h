#pragma once

#include "defs.h"
#include "event.h"

#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
    { 0x387477c1, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

struct efi_input_key {
    uint16_t scan_code;
    uint16_t unicode_char;
};

typedef struct efi_input_key* efi_input_key_t;

typedef struct efi_simple_text_input_protocol* efi_simple_text_input_t;

struct efi_simple_text_input_protocol {
    efi_status_t (efi_api *reset) (
        efi_simple_text_input_t self,
        bool extended_verification);
    
    efi_status_t (efi_api *read_key) (
        efi_simple_text_input_t self,
        efi_input_key_t key);

    efi_event_t wait_for_key;
};
