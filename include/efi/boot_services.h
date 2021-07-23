#pragma once

#include "defs.h"
#include "device_path.h"
#include "event.h"
#include "memory.h"

typedef efi_size_t efi_tpl_t;

enum efi_interface {
	EFI_NATIVE_INTERFACE,
};

typedef enum efi_interface efi_interface_t;

enum efi_locate_search {
    EFI_SEARCH_ALL_HANDLES,
    EFI_SEARCH_BY_REGISTER_NOTIFY,
    EFI_SEARCH_BY_PROTOCOL,
};

typedef enum efi_locate_search efi_locate_search_t;

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL          UINT32_C(0x00000001)
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL                UINT32_C(0x00000002)
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL               UINT32_C(0x00000004)
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER         UINT32_C(0x00000008)
#define EFI_OPEN_PROTOCOL_BY_DRIVER                   UINT32_C(0x00000010)
#define EFI_OPEN_PROTOCOL_EXCLUSIVE                   UINT32_C(0x00000020)

struct efi_protocol_information_entry {
    efi_handle_t agent_handle;
    efi_handle_t controller_handle;
    uint32_t attributes;
    uint32_t open_count;
};

#define EFI_BOOT_SERVICES_SIGNATURE UINT64_C(0x56524553544f4f42) /* "BOOTSERV" */

typedef struct efi_protocol_information_entry* efi_protocol_information_entry_t;

struct efi_boot_services_table {
        struct efi_table_header hdr;

        efi_tpl_t (*raise_tpl) (
			efi_tpl_t new_tpl
        );
        void (*restore_tpl) (
			efi_tpl_t old_tpl
        );

        efi_status_t (*allocate_pages) (
			efi_allocate_t type,
			efi_memory_t memory_type,
			efi_size_t pages,
			efi_physical_address_t* memory
        );

        efi_status_t (*free_pages) (
			efi_physical_address_t memory,
			efi_size_t pages
        );

        efi_status_t (*get_memory_map) (
			efi_size_t* memory_map_size,
			efi_memory_descriptor_t memory_map,
			efi_size_t* map_key,
			efi_size_t* descriptor_size,
			uint32_t* descriptor_version
        );

        efi_status_t (*allocate_pool) (
			efi_memory_t pool_type,
			efi_size_t size,
			void **buffer
        );

        efi_status_t (*free_pool) (
			void *buffer
        );

        efi_status_t (*create_event) (
			uint32_t type,
			efi_tpl_t notify_tpl,
			efi_event_notify notify_function,
			void* notify_context,
			efi_event_t event
        );

        efi_status_t (*set_timer) (
			efi_event_t event,
			efi_event_timer_delay_t type,
			uint64_t trigger_time
        );

        efi_status_t (*wait_for_event) (
			efi_size_t number_of_events,
			efi_event_t* event,
			efi_size_t *index
        );

        efi_status_t (*signal_event) (
			efi_event_t event
        );

        efi_status_t (*close_event) (
			efi_event_t event
        );

        efi_status_t (*check_event) (
			efi_event_t event
        );

        efi_status_t (*install_protocol_interface) (
			efi_handle_t* handle,
			efi_guid_t protocol,
            efi_interface_t interface_type,
			void *interface
        );

        efi_status_t (*reinstall_protocol_interface) (
            efi_handle_t handle,
            efi_guid_t protocol,
			void* old_interface,
			void* new_interface
        );

        efi_status_t (*uninstall_protocol_interface) (
			efi_handle_t handle,
			efi_guid_t protocol,
			void* interface
        );
        efi_status_t (*handle_protocol) (
			efi_handle_t handle,
			efi_guid_t protocol,
			void** interface
        );

        void *reserved;

        efi_status_t (*register_protocol_notify) (
			efi_guid_t protocol,
			efi_event_t event,
			void** registration
        );
        efi_status_t (*locate_handle) (
			efi_locate_search_t search_type,
			efi_guid_t protocol,
			void *search_key,
			efi_size_t* buffer_size,
			efi_handle_t* buffer
        );
        efi_status_t (*locate_device_path) (
			efi_guid_t protocol,
			efi_device_path_t* device_path,
			efi_handle_t* device
        );

        efi_status_t (*install_configuration_table) (
			efi_guid_t guid,
			void *table
        );

        efi_status_t (*load_image) (
			bool boot_policy,
			efi_handle_t parent_image_handle,
			efi_device_path_t device_path,
			void* source_buffer,
			efi_size_t source_size,
			efi_handle_t* image_handle
        );

        efi_status_t (*start_image) (
			efi_handle_t image_handle,
			efi_size_t* exit_data_size,
			char16_t** exit_data
        );

        efi_status_t (*exit) (
			efi_handle_t image_handle,
			efi_status_t exit_status,
			efi_size_t exit_data_size,
			char16_t* exit_data
        );

        efi_status_t (*unload_image) (
			efi_handle_t image_handle
        );

        [[ gnu::noreturn ]]
        efi_status_t (*exit_boot_services) (
			efi_handle_t image_handle,
			efi_size_t map_key
        );

        efi_status_t (*get_next_monotonic_count) (
			uint64_t* count
        );

        efi_status_t (*stall) (
			efi_size_t microseconds
        );

        efi_status_t (*set_watchdog_timer) (
			efi_size_t timeout,
			uint64_t watchdog_code,
			efi_size_t data_size,
			char16_t* watchdog_data
        );

        /* 1.1+ */

        efi_status_t (*connect_controller) (
			efi_handle_t controller_handle,
			efi_handle_t* driver_image_handle,
			efi_device_path_t* remaining_device_path,
			bool recursive
        );

        efi_status_t (*disconnect_controller) (
			efi_handle_t controller_handle,
			efi_handle_t driver_image_handle,
			efi_handle_t child_handle
        );

        efi_status_t (*open_protocol) (
			efi_handle_t handle,
			efi_guid_t protocol,
			void** interface,
			efi_handle_t agent_handle,
			efi_handle_t controller_handle,
			uint32_t attributes
        );

        efi_status_t (*close_protocol) (
            efi_handle_t handle,
            efi_guid_t protocol,
            efi_handle_t agent_handle,
            efi_handle_t controller_handle
        );

        efi_status_t (*open_protocol_information) (
            efi_handle_t handle,
            efi_guid_t protocol,
            efi_protocol_information_entry_t* entry_buffer,
            efi_size_t *entry_count
        );

        efi_status_t (*protocols_per_handle) (
            efi_handle_t handle,
            efi_guid_t** protocol_buffer,
            efi_size_t* protocol_buffer_count
        );

        efi_status_t (*locate_handle_buffer) (
			efi_locate_search_t search_type,
			efi_guid_t protocol,
			void* search_key,
			efi_size_t * no_handles,
			efi_handle_t ** buffer
        );

        efi_status_t (*locate_protocol) (
            efi_guid_t protocol,
            void *registration,
            void **interface
        );

        efi_status_t (*install_multiple_protocol_interfaces) (
			efi_handle_t *handle,
			...
        );

        efi_status_t (*uninstall_multiple_protocol_interfaces) (
			efi_handle_t handle,
			...
        );

        efi_status_t (*calculate_crc32) (
			void *data,
			efi_size_t data_size,
			uint32_t* crc32
        );

        void (*copy_mem) (
			void* destination,
			const void* source,
			efi_size_t length
        );

        void (*set_mem) (
			void* buffer,
			efi_size_t size,
			uint8_t value
        );

        /* 2.0+ */

        efi_status_t (*create_event_ex) (
            uint32_t type,
            efi_tpl_t notify_tpl,
            efi_event_notify notify_function,
            void *notify_context,
            efi_guid_t event_group,
            efi_event_t* event
        );
};

typedef struct efi_boot_services_table* efi_boot_services_table_t;
