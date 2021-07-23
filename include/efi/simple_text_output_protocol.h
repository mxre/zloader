#pragma once

#include "defs.h"

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
    { 0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

struct efi_simple_text_output_mode {
	int32_t max_mode;
	int32_t mode;
	int32_t attribute;
	int32_t cursor_column;
	int32_t cursor_row;
	bool cursor_visible;
};

typedef struct efi_simple_text_output_mode* efi_simple_text_output_mode_t;

#define EFI_BLACK             UINT8_C(0x00)
#define EFI_BLUE              UINT8_C(0x01)
#define EFI_GREEN             UINT8_C(0x02)
#define EFI_CYAN              UINT8_C(0x03)
#define EFI_RED               UINT8_C(0x04)
#define EFI_MAGENTA           UINT8_C(0x05)
#define EFI_BROWN             UINT8_C(0x06)
#define EFI_LIGHTGRAY         UINT8_C(0x07)
#define EFI_BRIGHT            UINT8_C(0x08)
#define EFI_DARKGRAY          (EFI_BLACK    | EFI_BRIGHT)
#define EFI_LIGHTBLUE         (EFI_BLUE     | EFI_BRIGHT)
#define EFI_LIGHTGREEN        (EFI_GREEN    | EFI_BRIGHT)
#define EFI_LIGHTCYAN         (EFI_CYAN     | EFI_BRIGHT)
#define EFI_LIGHTRED          (EFI_RED      | EFI_BRIGHT)
#define EFI_LIGHTMAGENTA      (EFI_MAGENTA  | EFI_BRIGHT)
#define EFI_YELLOW            (EFI_BROWN    | EFI_BRIGHT)
#define EFI_WHITE             UINT8_C(0x0f)

#define EFI_BACKGROUND_BLACK          UINT8_C(0x00)
#define EFI_BACKGROUND_BLUE           UINT8_C(0x10)
#define EFI_BACKGROUND_GREEN          UINT8_C(0x20)
#define EFI_BACKGROUND_CYAN           UINT8_C(0x30)
#define EFI_BACKGROUND_RED            UINT8_C(0x40)
#define EFI_BACKGROUND_MAGENTA        UINT8_C(0x50)
#define EFI_BACKGROUND_BROWN          UINT8_C(0x60)
#define EFI_BACKGROUND_LIGHTGRAY      UINT8_C(0x70)

#define EFI_TEXT_ATTR(f,b)  ((f) | ((b) << 4))

typedef struct efi_simple_text_output_protocol* efi_simple_text_output_protocol_t;

struct efi_simple_text_output_protocol {
    efi_api
	efi_status_t (*reset)(
		efi_simple_text_output_protocol_t self,
		bool extended_verification);

    efi_api
	efi_status_t (*output_string)(
		efi_simple_text_output_protocol_t self,
		const char16_t* string);

    efi_api
	efi_status_t (*test_string)(
		efi_simple_text_output_protocol_t self,
		const char16_t* string);

    efi_api
	efi_status_t (*query_mode)(
		efi_simple_text_output_protocol_t self,
		efi_size_t mode_number,
		efi_size_t* columns,
		efi_size_t* rows);

    efi_api
	efi_status_t (*set_mode)(
		efi_simple_text_output_protocol_t self,
		efi_size_t mode_number);

    efi_api
	efi_status_t (*set_attribute)(
		efi_simple_text_output_protocol_t self,
		efi_size_t attribute);

    efi_api
	efi_status_t (*clear_screen)(
		efi_simple_text_output_protocol_t self);

    efi_api
	efi_status_t (*set_cursor_position)(
		efi_simple_text_output_protocol_t self,
		efi_size_t column,
		efi_size_t row);

    efi_api
	efi_status_t (*enable_cursor)(
		efi_simple_text_output_protocol_t self,
		bool visible);

    efi_api
	efi_simple_text_output_mode_t mode;
};
