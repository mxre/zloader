#pragma once

#include "defs.h"

#define EFI_EVENT_TIMER                         UINT32_C(0x80000000)
#define EFI_EVENT_RUNTIME                       UINT32_C(0x40000000)
#define EFI_EVENT_RUNTIME_CONTEXT               UINT32_C(0x20000000)
#define EFI_EVENT_NOTIFY_WAIT                   UINT32_C(0x00000100)
#define EFI_EVENT_NOTIFY_SIGNAL                 UINT32_C(0x00000200)
#define EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES     UINT32_C(0x00000201)
#define EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE UINT32_C(0x60000202)
#define EFI_EVENT_EFI_SIGNAL_MASK               UINT32_C(0x000000FF)
#define EFI_EVENT_EFI_SIGNAL_MAX                4

typedef void* efi_event_t;

typedef void (efi_api *efi_event_notify) (efi_event_t event, void *context);

enum efi_event_timer_delay {
        EFI_TIMER_CANCEL,
        EFI_TIMER_PERIODIC,
        EFI_TIMER_RELATIVE,
};

typedef enum efi_event_timer_delay efi_event_timer_delay_t;
