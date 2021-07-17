#pragma once

#include <efi.h>

#include "externs.h"

static inline
efi_status_t print(
    const char16_t* string
) {
    return ST->out->output_string(ST->out, string);
}

#ifdef EFI_PRINTF

#include <stdargs.h>

efi_size_t _iprint(
    efi_size_t column,
    efi_size_t row,
    efi_simple_text_output_protocol_t out,
    const char16_t* fmt,
    const char8_t* fmta,
    va_list args
);

static inline
efi_size_t printf(
    const char16_t* fmt,
    ...
) {
    va_list args;
    efi_size_t ret;
    va_start(args, fmt);
    ret = _iprint((efi_size_t) -1, (efi_size_t) -1, ST->out, fmt, NULL, args);
    va_end(args);
    return ret;
}

static inline
efi_size_t printf_at(
    efi_size_t column,
    efi_size_t row,
    const char16_t* fmt,
    ...
) {
    va_list args;
    efi_size_t ret;
    va_start(args, fmt);
    ret = _iprint(column, row, ST->out, fmt, NULL, args);
    va_end(args);
    return ret;
}

static inline
efi_size_t aprintf(
    const char8_t* fmt,
    ...
) {
    va_list args;
    efi_size_t ret;
    va_start(args, fmt);
    ret = _iprint((efi_size_t) -1, (efi_size_t) -1, ST->out, NULL, fmt, args);
    va_end(args);
    return ret;
}

char16_t*  value_to_string(
    char16_t* buffer,
    int64_t value
);

char16_t*  value_to_hex_string(
    char16_t* buffer,
    uint64_t value
);

#ifdef EFI_FLOATING_POINT
char16_t* float_to_string (
    char16_t* buffer,
    double value
);
#endif

char16_t* time_to_string (
    char16_t* buffer,
    efi_time_t time
);

#endif /* EFI_PRINTF */
