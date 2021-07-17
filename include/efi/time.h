#pragma once

#include "defs.h"

struct efi_time {          
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t __pad1;
    uint32_t nanosecond;
    uint16_t timezone;
    uint8_t daylight;
    uint8_t __pad2;
};

typedef struct efi_time* efi_time_t;

// Bit definitions for EFI_TIME.Daylight
#define EFI_TIME_ADJUST_DAYLIGHT    UINT8_C(0x01)
#define EFI_TIME_IN_DAYLIGHT        UINT8_C(0x02)

// Value definition for EFI_TIME.TimeZone
#define EFI_UNSPECIFIED_TIMEZONE    INT16_C(0x07FF)
