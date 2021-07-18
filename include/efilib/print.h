/**
 * @file print.h
 * @author Max Resch
 * @brief printf implementation based on GNU-EFI Print function
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 *  
 * 
 *   %w.lF   -   w = width
 *               l = field width
 *               F = format of arg
 * 
 * Args modifing F:
 *   0       -   pad with zeros
 *   -       -   justify on left (default is on right)
 *   *       -   width provided on stack
 *   n       -   Set output attribute to normal (for this field only)
 *   b       -   Set output attribute to highlight (for this field only)
 *   e       -   Set output attribute to error (for this field only)
 *   l       -   Value is 64 bits, or char/string is wide
 *   h       -   Value is 16 bits
 *   z       -   Value is of size_t (32 or 64 bits)
 *   N       -   Set output attribute to normal
 *   B       -   Set output attribute to highlight
 *   E       -   Set output attribute to error
 * 
 * Args F:
 *   s       -   string
 *   X       -   fixed 8 byte value in hex
 *   x       -   hex value
 *   d       -   value as signed decimal
 *   i       -   same as d
 *   u       -   value as unsigned decimal
 *   f       -   value as floating point
 *   c       -   char
 *   t       -   EFI time structure (printed as ISO 8601)
 *   g       -   GUID (from pointer)
 *   r       -   EFI status message (from status code)
 *
 *   %       -   Print a %
 */

#include <efi.h>

#include "externs.h"

/**
 * @brief simple print to console
 * 
 * @note line ending is `\r\n` an is not added by this function
 */
static inline
efi_status_t print(
    const char16_t* string
) {
    return ST->out->output_string(ST->out, string);
}

/**
 * @brief color for normal text
 */
#define EFILIB_PRINT_NORMAL_COLOR      EFI_LIGHTGRAY

/**
 * @brief color for highlighted text
 */
#define EFILIB_PRINT_HIGHLIGHT_COLOR   EFI_YELLOW

/**
 * @brief color for error text
 */
#define EFILIB_PRINT_ERROR_COLOR       EFI_LIGHTRED

#ifdef EFILIB_PRINTF

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
efi_size_t wprintf(
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
efi_size_t wprintf_at(
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
efi_size_t printf(
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

#ifdef EFILIB_FLOATING_POINT
char16_t* float_to_string (
    char16_t* buffer,
    double value
);
#endif

char16_t* time_to_string (
    char16_t* buffer,
    efi_time_t time
);

#endif /* EFILIB_PRINTF */
