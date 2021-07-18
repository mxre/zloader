/**
 * @file pe.h
 * @author Max Resch
 * @brief PE image header
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 *
 * @see https://docs.microsoft.com/windows/win32/debug/pe-format
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "util.h"

#define MZ_DOS_SIGNATURE                    UINT16_C(0x5a4d)     /* MZ */

/** 
 * COFF file header
 */
struct __packed PE_COFF_header {
    uint16_t machine;
    uint16_t number_of_sections;
    uint32_t time_date_stamp;
    uint32_t pointer_to_symbol_table;
    uint32_t number_of_symbols;
    uint16_t size;
    uint16_t characteristics;
};

typedef struct PE_COFF_header* PE_COFF_header_t;

#define PE_HEADER_MACHINE_UNKNOWN       UINT16_C(0x0)    ///< The content of this field is assumed to be applicable to any machine type
#define PE_HEADER_MACHINE_AM33          UINT16_C(0x1d3)  ///< Matsushita AM33
#define PE_HEADER_MACHINE_AMD64         UINT16_C(0x8664) ///< x64
#define PE_HEADER_MACHINE_ARM           UINT16_C(0x1c0)  ///< ARM little endian
#define PE_HEADER_MACHINE_ARM64         UINT16_C(0xaa64) ///< ARM64 little endian
#define PE_HEADER_MACHINE_ARMNT         UINT16_C(0x1c4)  ///< ARM Thumb-2 little endian
#define PE_HEADER_MACHINE_EBC           UINT16_C(0xebc)  ///< EFI byte code
#define PE_HEADER_MACHINE_I386          UINT16_C(0x14c)  ///< Intel 386 or later processors and compatible processors
#define PE_HEADER_MACHINE_IA64          UINT16_C(0x200)  ///< Intel Itanium processor family
#define PE_HEADER_MACHINE_M32R          UINT16_C(0x9041) ///< Mitsubishi M32R little endian
#define PE_HEADER_MACHINE_MIPS16        UINT16_C(0x266)  ///< MIPS16
#define PE_HEADER_MACHINE_MIPSFPU       UINT16_C(0x366)  ///< MIPS with FPU
#define PE_HEADER_MACHINE_MIPSFPU16     UINT16_C(0x466)  ///< MIPS16 with FPU
#define PE_HEADER_MACHINE_POWERPC       UINT16_C(0x1f0)  ///< Power PC little endian
#define PE_HEADER_MACHINE_POWERPCFP     UINT16_C(0x1f1)  ///< Power PC with floating point support
#define PE_HEADER_MACHINE_R4000         UINT16_C(0x166)  ///< MIPS little endian
#define PE_HEADER_MACHINE_RISCV32       UINT16_C(0x5032) ///< RISC-V 32-bit address space
#define PE_HEADER_MACHINE_RISCV64       UINT16_C(0x5064) ///< RISC-V 64-bit address space
#define PE_HEADER_MACHINE_RISCV128      UINT16_C(0x5128) ///< RISC-V 128-bit address space
#define PE_HEADER_MACHINE_SH3           UINT16_C(0x1a2)  ///< Hitachi SH3
#define PE_HEADER_MACHINE_SH3DSP        UINT16_C(0x1a3)  ///< Hitachi SH3 DSP
#define PE_HEADER_MACHINE_SH4           UINT16_C(0x1a6)  ///< Hitachi SH4
#define PE_HEADER_MACHINE_SH5           UINT16_C(0x1a8)  ///< Hitachi SH5
#define PE_HEADER_MACHINE_THUMB         UINT16_C(0x1c2)  ///< Thumb
#define PE_HEADER_MACHINE_WCEMIPSV2     UINT16_C(0x169)  ///< MIPS little-endian WCE v2

#if __x86_64__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_AMD64
#elif __i386__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_I386
#elif __aarch64__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_ARM64
#elif __arm__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_ARM
#elif __riscv && __riscv_xlen == 64
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_RISCV64
#elif __riscv && __riscv_xlen == 64
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_RISCV32
#else
#error "Unkown target CPU consider adding it to pe.h"
#endif

#define PE_HEADER_RELOCS_STRIPPED      UINT16_C(0x0001)   ///< 0x0001  Relocation info stripped from file.
#define PE_HEADER_EXECUTABLE_IMAGE     UINT16_C(0x0002)   ///< 0x0002  File is executable  (i.e. no unresolved externel references).
#define PE_HEADER_LINE_NUMS_STRIPPED   UINT16_C(0x0004)   ///< 0x0004  Line nunbers stripped from file.
#define PE_HEADER_LOCAL_SYMS_STRIPPED  UINT16_C(0x0008)   ///< 0x0008  Local symbols stripped from file.
#define PE_HEADER_BYTES_REVERSED_LO    UINT16_C(0x0010)   ///< 0x0080  Bytes of machine word are reversed.
#define PE_HEADER_32BIT_MACHINE        UINT16_C(0x0020)   ///< 0x0100  32 bit word machine.
#define PE_HEADER_DEBUG_STRIPPED       UINT16_C(0x0040)   ///< 0x0200  Debugging info stripped from file in .DBG file.
#define PE_HEADER_SYSTEM               UINT16_C(0x0080)   ///< 0x1000  System File.
#define PE_HEADER_DLL                  UINT16_C(0x0100)   ///< 0x2000  File is a DLL.
#define PE_HEADER_BYTES_REVERSED_HI    UINT16_C(0x0200)   ///< 0x8000  Bytes of machine word are reversed.

/**
 * Directory Entries
 */
#define PE_HEADER_DIRECTORY_ENTRY_EXPORT      0
#define PE_HEADER_DIRECTORY_ENTRY_IMPORT      1
#define PE_HEADER_DIRECTORY_ENTRY_RESOURCE    2
#define PE_HEADER_DIRECTORY_ENTRY_EXCEPTION   3
#define PE_HEADER_DIRECTORY_ENTRY_SECURITY    4
#define PE_HEADER_DIRECTORY_ENTRY_BASERELOC   5
#define PE_HEADER_DIRECTORY_ENTRY_DEBUG       6
#define PE_HEADER_DIRECTORY_ENTRY_COPYRIGHT   7
#define PE_HEADER_DIRECTORY_ENTRY_GLOBALPTR   8
#define PE_HEADER_DIRECTORY_ENTRY_TLS         9
#define PE_HEADER_DIRECTORY_ENTRY_LOAD_CONFIG 10

#define PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES 16

/**
 * @brief Header Data Directories.
 */
struct __packed PE_header_data_directory {
    uint32_t virtual_address;
    uint32_t size;
};

typedef struct PE_header_data_directory* PE_header_data_directory_t;

/**
 * @brief 8bit version fields
 */
struct __packed PE_version8 {
    uint8_t major;
    uint8_t minor;
};

/**
 * @brief 16bit version fields
 */
struct __packed PE_version16 {
    uint16_t major;
    uint16_t minor;
};

#define PE_HEADER_OPTIONAL_HDR32_MAGIC 0x10b

/**
 * @brief Optional COFF Header Fields for PE32. 
 */
struct __packed PE_optional_header32 {
    /* Standard fields. */
    struct   PE_version8 linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    uint32_t base_of_data;                       ///< PE32 contains this additional field, which is absent in PE32+.

    /* Optional Header Windows-Specific Fields. */
    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    struct   PE_version16 operation_system_version;
    struct   PE_version16 image_version;
    struct   PE_version16 subsystem_version;
    uint32_t win32_version_value;               ///< reserved always 0
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t check_sum;
    uint16_t subsystem;
    uint16_t DLL_characteristics;
    uint32_t size_of_stack_reserve;
    uint32_t size_of_stack_commit;
    uint32_t size_of_heap_reserve;
    uint32_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_RVA_and_sizes;
    struct   PE_header_data_directory data_directory[PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES];
};

typedef struct PE_optional_header32 PE_optional_header32_t;

#define PE_HEADER_OPTIONAL_HDR64_MAGIC 0x20b

/**
 * @brief Optional COFF Header Fields for PE32+. 
 */
struct __packed PE_optional_header64 {
/* Standard fields. */
    struct   PE_version8 linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;

    /* Optional Header Windows-Specific Fields. */
    uint64_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    struct   PE_version16 operation_system_version;
    struct   PE_version16 image_version;
    struct   PE_version16 subsystem_version;
    uint32_t win32_version_value;               ///< reserved always 0
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t check_sum;
    uint16_t subsystem;
    uint16_t DLL_characteristics;
    uint64_t size_of_stack_reserve;
    uint64_t size_of_stack_commit;
    uint64_t size_of_heap_reserve;
    uint64_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_RVA_and_sizes;
    struct   PE_header_data_directory data_directory[PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES];
};

typedef struct PE_optional_header64 PE_optional_header64_t;

#define PE_HEADER_SUBSYSTEM_UNKNOWN                     0  ///< An unknown subsystem
#define PE_HEADER_SUBSYSTEM_NATIVE                      1  ///< Device drivers and native Windows processes
#define PE_HEADER_UBSYSTEM_WINDOWS_GUI                  2  ///< The Windows graphical user interface (GUI) subsystem
#define PE_HEADER_SUBSYSTEM_WINDOWS_CUI                 3  ///< The Windows character subsystem
#define PE_HEADER_SUBSYSTEM_OS2_CUI                     5  ///< The OS/2 character subsystem
#define PE_HEADER_SUBSYSTEM_POSIX_CUI                   7  ///< The Posix character subsystem
#define PE_HEADER_SUBSYSTEM_NATIVE_WINDOWS              8  ///< Native Win9x driver
#define PE_HEADER_SUBSYSTEM_WINDOWS_CE_GUI              9  ///< Windows CE
#define PE_HEADER_SUBSYSTEM_EFI_APPLICATION             10 ///< An Extensible Firmware Interface (EFI) application
#define PE_HEADER_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER     11 ///< An EFI driver with boot services
#define PE_HEADER_SUBSYSTEM_EFI_RUNTIME_DRIVER          12 ///< An EFI driver with run-time services
#define PE_HEADER_SUBSYSTEM_EFI_ROM                     13 ///< An EFI ROM image
#define PE_HEADER_SUBSYSTEM_XBOX                        14 ///< XBOX
#define PE_HEADER_SUBSYSTEM_WINDOWS_BOOT_APPLICATION    16 ///< Windows boot application.

#define PE_HEADER_SIGNATURE                 UINT32_C(0x00004550) /* PE\0\0*/

/** 
 * @brief PE header
 */
struct __packed PE_image_headers {
    uint32_t signature;
    struct   PE_COFF_header file_header;
    uint16_t magic;
    union {
        struct   PE_optional_header32 optional_header32;
        struct   PE_optional_header64 optional_header64;
    };
};

typedef struct PE_image_headers* PE_image_headers_t;

/**
 * @brief Length of section name 
 */
#define PE_SECTION_SIZE_OF_SHORT_NAME 8

/**
 * @brief Windows only alows 96 sections
 */
#define PE_HEADER_MAX_NUMBER_OF_SECTIONS

/**
 * @brief Section Table. This table immediately follows the optional header
 */
struct __packed PE_header_section_header {
    char8_t name[PE_SECTION_SIZE_OF_SHORT_NAME];
    union {
        uint32_t physical_address;
        uint32_t virtual_size;
    };
    uint32_t virtual_address;
    uint32_t size_of_raw_data;
    uint32_t pointer_to_raw_data;
    uint32_t pointer_to_relocations;
    uint32_t pointer_to_linenumbers;
    uint32_t number_of_relocations;
    uint32_t number_of_linenumbers;
    uint32_t characteristics;
};

typedef struct PE_section_header* PE_section_t;

/* Section header characteristics */

#define PE_SECTION_TYPE_NO_PAD               0x00000008 ///< Reserved.
#define PE_SECTION_CNT_CODE                  0x00000020
#define PE_SECTION_CNT_INITIALIZED_DATA      0x00000040
#define PE_SECTION_CNT_UNINITIALIZED_DATA    0x00000080
                                                   
#define PE_SECTION_LNK_OTHER                 0x00000100 ///< Reserved.
#define PE_SECTION_LNK_INFO                  0x00000200 ///< Section contains comments or some other type of information.
#define PE_SECTION_LNK_REMOVE                0x00000800 ///< Section contents will not become part of image.
#define PE_SECTION_LNK_COMDAT                0x00001000
                                                   
#define PE_SECTION_ALIGN_1BYTES              0x00100000
#define PE_SECTION_ALIGN_2BYTES              0x00200000
#define PE_SECTION_ALIGN_4BYTES              0x00300000
#define PE_SECTION_ALIGN_8BYTES              0x00400000
#define PE_SECTION_ALIGN_16BYTES             0x00500000
#define PE_SECTION_ALIGN_32BYTES             0x00600000
#define PE_SECTION_ALIGN_64BYTES             0x00700000
                                              
#define PE_SECTION_MEM_DISCARDABLE           0x02000000
#define PE_SECTION_MEM_NOT_CACHED            0x04000000
#define PE_SECTION_MEM_NOT_PAGED             0x08000000
#define PE_SECTION_MEM_SHARED                0x10000000
#define PE_SECTION_MEM_EXECUTE               0x20000000
#define PE_SECTION_MEM_READ                  0x40000000
#define PE_SECTION_MEM_WRITE                 0x80000000

/**
 * @brief check if the data in data is a PE image
 * 
 * @param[in] buffer
 * @param[in] buffer_size 
 * 
 * @return 0 or offset to PE header
 */
static inline
size_t PE_header(simple_buffer_t buffer) {
    /* has MZ magic and is large enough for MSDOS header */
    if (*(uint16_t*) buffer_pos(buffer) == MZ_DOS_SIGNATURE && (buffer->length - buffer->pos) > 0x40) {
        uint32_t pe_offset = *(uint32_t*)((uint8_t*) buffer_pos(buffer) + 0x3c);
        /* check if large enough for PE header */
        if ((buffer->length - buffer->pos) > (pe_offset + 0x14)) {
            if(*(uint32_t*)((uint8_t*) buffer_pos(buffer) + pe_offset) == PE_HEADER_SIGNATURE) {
                return pe_offset;
            }
        }
    }
    return 0;
}

struct locate_pe_sections {
    /**
     * @brief pointer to virtual address where section is loaded
     */
    void* load_address;

    /**
     * @brief offset from the image base address
     */
    size_t offset;

    /**
     * @brief length of the sections data
     */
    size_t size;

    /**
     * @brief section short name
     */
    char8_t name[PE_SECTION_SIZE_OF_SHORT_NAME]; 
};

typedef struct locate_pe_sections* locate_pe_sections_t;

bool pe_locate_sections(
    const simple_buffer_t image,
    locate_pe_sections_t sections
);
