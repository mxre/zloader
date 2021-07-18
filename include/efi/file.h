#pragma once

#include "defs.h"
#include "time.h"

typedef struct efi_file_protocol* efi_file_protocol_t;
typedef struct efi_file_protocol* efi_file_handle_t;

#define EFI_FILE_PROTOCOL_REVISION         UINT64_C(0x00010000)
#define EFI_FILE_HANDLE_REVISION           EFI_FILE_PROTOCOL_REVISION

// Open modes
#define EFI_FILE_MODE_READ      UINT64_C(0x0000000000000001)
#define EFI_FILE_MODE_WRITE     UINT64_C(0x0000000000000002)
#define EFI_FILE_MODE_CREATE    UINT64_C(0x8000000000000000)

// File attributes
#define EFI_FILE_READ_ONLY      UINT64_C(0x0000000000000001)
#define EFI_FILE_HIDDEN         UINT64_C(0x0000000000000002)
#define EFI_FILE_SYSTEM         UINT64_C(0x0000000000000004)
#define EFI_FILE_RESERVIED      UINT64_C(0x0000000000000008)
#define EFI_FILE_DIRECTORY      UINT64_C(0x0000000000000010)
#define EFI_FILE_ARCHIVE        UINT64_C(0x0000000000000020)
#define EFI_FILE_VALID_ATTR     UINT64_C(0x0000000000000037)

struct efi_file_protocol {
    uint64_t revision;

    efi_status_t (*open) (
        efi_file_handle_t self,
        efi_file_handle_t *new_file,
        const char16_t* filename,
        uint64_t mode,
        uint64_t attributes 
    );

    efi_status_t (*close) (
        efi_file_handle_t self
    );

    /* delete is a reserved name */
    efi_size_t (*unlink) (
        efi_file_handle_t self
    );

    efi_size_t (*read) (
        efi_file_handle_t self,
        efi_size_t* size,
        void* buffer
    );

    /**
     * @brief Writes data to a file.
     * 
     * @param self
     * @param[in,out] size
     *  On input, the size of the `buffer`. On output, the amount of data
     *  actually written. In both cases, the size is measured in bytes.
     * @param[in] buffer
     *  The buffer of data to write.
     * 
     * @retval EFI_SUCCESS          The data was written.
     * @retval EFI_UNSUPPORT        Writes to open directory files are not supported.
     * @retval EFI_NO_MEDIA         Writes to open directory files are not supported.
     * @retval EFI_DEVICE_ERROR     The device reported an error.
     * @retval EFI_DEVICE_ERROR     An attempt was made to write to a deleted file.
     * @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
     * @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
     * @retval EFI_ACCESS_DENIED    The file was opened read only.
     * @retval EFI_VOLUME_FULL      The volume is full.
     * 
     * @details
     *  The `write()` function writes the specified number of bytes to the file at the current
     *  file position. The current file position is advanced the actual number of bytes written,
     *  which is returned in `size`. Partial writes only occur when there has been a data error
     *  during the write attempt (such as "file space full"). The file is automatically grown
     *  to hold the data if required.
     * 
     *  Direct writes to opened directories are not supported.
     */
    efi_status_t (*write) (
        efi_file_handle_t self,
        efi_size_t* size,
        const void* buffer
    );

    efi_status_t (*get_position) (
        efi_file_handle_t self,
        efi_size_t* pos
    );

    efi_status_t (*set_position) (
        efi_file_handle_t self,
        efi_size_t* pos
    );

    efi_status_t (*get_info) (
        efi_file_handle_t self,
        efi_guid_t type,
        efi_size_t* size,
        void* buffer
    );

    efi_status_t (*set_info) (
        efi_file_handle_t self,
        efi_guid_t type,
        efi_size_t size,
        void* buffer
    );

    efi_status_t (*flush) (
        efi_file_handle_t self
    );
};

#define EFI_FILE_INFO_GUID   \
    { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

struct efi_file_info {
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    struct efi_time create_time;
    struct efi_time last_access_time;
    struct efi_time modification_time;
    uint64_t attribute;
    char16_t filename[1];
};

typedef struct efi_file_info* efi_file_info_t;

#define EFI_FILE_SYSTEM_INFO_GUID    \
    { 0x9576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

struct efi_file_system_info {
    uint64_t size;
    bool read_only;
    uint64_t volume_size;
    uint64_t free_space;
    uint32_t block_size;
    char16_t volume_label[1];
};

typedef struct efi_file_system_info* efi_file_system_info_t;
