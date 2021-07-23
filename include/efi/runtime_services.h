#pragma once

#include "defs.h"
#include "time.h"
#include "memory.h"
#include "capsule.h"

struct efi_time_capabilities {
    uint32_t resolution; ///< 1e-6 parts per million
    uint32_t accuracy;   ///< hertz
    bool sets_to_zero;   ///< Set clears sub-second time
};

typedef struct efi_time_capabilities* efi_time_capabilities_t;

/* Pointer Debug Disposition */

#define EFI_OPTIONAL_PTR UINT32_C(0x00000001)
#define EFI_INTERNAL_FNC UINT32_C(0x00000002)  ///< Pointer to internal runtime fnc
#define EFI_INTERNAL_PTR UINT32_C(0x00000004)  ///< Pointer to internal runtime data

enum efi_reset_type {
    EFI_RESET_COLD,
    EFI_RESET_WARM,
    EFI_RESET_SHUTDOWN
};

typedef enum efi_reset_type efi_reset_type_t;

#define EFI_GLOBAL_VARIABLE_GUID \
    { 0x8BE4DF61, 0x93CA, 0x11d2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C} }

/* Variable Attributes */

#define EFI_VARIABLE_NON_VOLATILE                           UINT32_C(0x00000001)
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                     UINT32_C(0x00000002)
#define EFI_VARIABLE_RUNTIME_ACCESS                         UINT32_C(0x00000004)
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                  UINT32_C(0x00000008)
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS             UINT32_C(0x00000010) ///< depricated
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS  UINT32_C(0x00000020)
#define EFI_VARIABLE_APPEND_WRITE                           UINT32_C(0x00000040)

/**
 * This attribute indicates that the variable payload begins
 * with an `efi_variable_authentication_3` structure, and
 * potentially more structures as indicated by fields of this 
 */
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS          UINT32_C(0x00000080)

#define EFI_VARIABLE_AUTHENTICATION_3_TIMESTAMP_TYPE 1
#define EFI_VARIABLE_AUTHENTICATION_3_NONCE_TYPE     2

struct __packed efi_variable_authentication_3 {
    uint8_t version;
    uint8_t type;
    uint32_t metadata_size;
    uint32_t flags;
};

typedef struct efi_variable_authentication_3 efi_variable_authentication_3_t;

#define EFI_VARIABLE_AUTHENTICATION_3_CERT_ID_SHA256 1

struct __packed efi_variable_authentication_3_cert_id {
    uint8_t type;
    uint32_t id_size;
    uint8_t id[];  ///< data of length id_size
};

typedef struct efi_variable_authentication_3_cert_id efi_variable_authentication_3_cert_id_t;

#define EFI_RUNTIME_SERVICES_SIGNATURE UINT64_C(0x56524553544e5552) ///< "RUNTSERV"

typedef struct efi_runtime_services_table* efi_runtime_services_table_t;

struct efi_runtime_services_table  {
    struct efi_table_header hdr;

    /* Time services */

    efi_api
    efi_status_t (*get_time) (
        efi_time_t time,
        efi_time_capabilities_t capabilities
    );

    efi_api
    efi_status_t (*set_time) (
        const efi_time_t time
    );

    efi_api
    efi_status_t (*get_wakeup_time) (
        bool* enabled,
        bool* pending,
        efi_time_t time
    );

    efi_api
    efi_status_t (*set_wakeup_time) (
        bool enabled,
        const efi_time_t time
    );

    /* Virtual memory services */

    efi_api
    efi_status_t (*set_virtual_address_map) (
        efi_size_t memory_map_size,
        efi_size_t descriptor_size,
        uint32_t descriptor_version,
        efi_memory_descriptor_t virtual_map
    );

    efi_api
    efi_status_t (*convert_pointer) (
        efi_size_t debug_disposition,
        void** address
    );

    /* Variable services */

    efi_api
    efi_status_t (*get_variable) (
        const char16_t* variable_name,
        const efi_guid_t vendor_guid,
        uint32_t* attributes,
        efi_size_t* data_size,
        void* data
    );

    efi_api
    efi_status_t (*get_next_variable_name) (
        efi_size_t* variable_name_size,
        char16_t* variable_name,
        efi_guid_t vendor_guid
    );

    efi_api
    efi_status_t (*set_variable) (
        const char16_t* variable_name,
        const efi_guid_t vendor_guid,
        uint32_t attributes,
        efi_size_t data_size,
        const void* data
    );

    /* Misc */

    efi_api
    efi_status_t (*get_next_high_monotonic_count) (
        uint32_t* high_count
    );
    
    efi_api noreturn
    efi_status_t (*reset_system) (
        efi_reset_type_t reset_type,
        efi_status_t reset_status,
        efi_size_t data_size,
        const char16_t* data
    );

    /* Capsule Service UEFI 2.0+ */

    efi_api
    efi_status_t (*update_capsule) (
        const efi_capsule_header_t* capsule_header_array,
        efi_size_t capsule_count,
        efi_physical_address_t scatter_gather_list
    );

    efi_api
    efi_status_t (*query_capsule_capabilities) (
        const efi_capsule_header_t* capsule_header_array,
        efi_size_t capsule_count,
        uint64_t* maximum_capsule_size,
        efi_reset_type_t* reset_type
    );

    /* UEFI 2.0+ */

    efi_api
    efi_status_t (*query_variable_info) (
        uint32_t attributes,
        uint64_t* maximum_variable_storage_size,
        uint64_t* remaining_variable_storageSize,
        uint64_t* maximum_variable_size
    );
};
