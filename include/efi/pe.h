#pragma once

#include "compiler.h"
#include <stdint.h>

#define MZ_DOS_SIGNATURE UINT16_C(0x5a4d)     /* MZ */

#define DOS_PE_OFFSET_LOCATION 0x3C

/**
 * COFF file header
 */
struct __packed PE_COFF_header {
    uint32_t signature;
    uint16_t machine;
    uint16_t number_of_sections;
    uint32_t time_date_stamp;
    uint32_t pointer_to_symbol_table;
    uint32_t number_of_symbols;
    uint16_t size_of_optional_header;
    uint16_t characteristics;
};

typedef struct PE_COFF_header* PE_COFF_header_t;

#define PE_HEADER_MACHINE_UNKNOWN       UINT16_C(0x0000) ///< The content of this field is assumed to be applicable to any machine type
#define PE_HEADER_MACHINE_AM33          UINT16_C(0x01d3) ///< Matsushita AM33
#define PE_HEADER_MACHINE_AMD64         UINT16_C(0x8664) ///< x64
#define PE_HEADER_MACHINE_ARM           UINT16_C(0x01c0) ///< ARM little endian
#define PE_HEADER_MACHINE_ARM64         UINT16_C(0xaa64) ///< ARM64 little endian
#define PE_HEADER_MACHINE_ARMNT         UINT16_C(0x01c4) ///< ARM Thumb-2 little endian
#define PE_HEADER_MACHINE_EBC           UINT16_C(0x0ebc) ///< EFI byte code
#define PE_HEADER_MACHINE_I386          UINT16_C(0x014c) ///< Intel 386 or later processors and compatible processors
#define PE_HEADER_MACHINE_IA64          UINT16_C(0x0200) ///< Intel Itanium processor family
#define PE_HEADER_MACHINE_M32R          UINT16_C(0x9041) ///< Mitsubishi M32R little endian
#define PE_HEADER_MACHINE_MIPS16        UINT16_C(0x0266) ///< MIPS16
#define PE_HEADER_MACHINE_MIPSFPU       UINT16_C(0x0366) ///< MIPS with FPU
#define PE_HEADER_MACHINE_MIPSFPU16     UINT16_C(0x0466) ///< MIPS16 with FPU
#define PE_HEADER_MACHINE_POWERPC       UINT16_C(0x01f0) ///< Power PC little endian
#define PE_HEADER_MACHINE_POWERPCFP     UINT16_C(0x01f1) ///< Power PC with floating point support
#define PE_HEADER_MACHINE_R4000         UINT16_C(0x0166) ///< MIPS little endian
#define PE_HEADER_MACHINE_RISCV32       UINT16_C(0x5032) ///< RISC-V 32-bit address space
#define PE_HEADER_MACHINE_RISCV64       UINT16_C(0x5064) ///< RISC-V 64-bit address space
#define PE_HEADER_MACHINE_RISCV128      UINT16_C(0x5128) ///< RISC-V 128-bit address space
#define PE_HEADER_MACHINE_SH3           UINT16_C(0x01a2) ///< Hitachi SH3
#define PE_HEADER_MACHINE_SH3DSP        UINT16_C(0x01a3) ///< Hitachi SH3 DSP
#define PE_HEADER_MACHINE_SH4           UINT16_C(0x01a6) ///< Hitachi SH4
#define PE_HEADER_MACHINE_SH5           UINT16_C(0x01a8) ///< Hitachi SH5
#define PE_HEADER_MACHINE_THUMB         UINT16_C(0x01c2) ///< Thumb
#define PE_HEADER_MACHINE_WCEMIPSV2     UINT16_C(0x0169) ///< MIPS little-endian WCE v2

#if __x86_64__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_AMD64
#elif __i386__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_I386
#elif __aarch64__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_ARM64
#elif __arm__
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_THUMB /* according to UEFI specs */
#elif __riscv && __riscv_xlen == 64
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_RISCV64
#elif __riscv && __riscv_xlen == 32
#define PE_HEADER_MACHINE_NATIVE PE_HEADER_MACHINE_RISCV32
#else
#error "Unkown target CPU consider adding it to pe.h"
#endif

#define PE_HEADER_RELOCS_STRIPPED       UINT16_C(0x0001)    ///< Relocation info stripped from file.
#define PE_HEADER_EXECUTABLE_IMAGE      UINT16_C(0x0002)    ///< File is executable  (i.e. no unresolved externel references).
#define PE_HEADER_LINE_NUMS_STRIPPED    UINT16_C(0x0004)    ///< Line nunbers stripped from file.
#define PE_HEADER_LOCAL_SYMS_STRIPPED   UINT16_C(0x0008)    ///< Local symbols stripped from file.
#define PE_HEADER_AGGRESSIVE_WS_TRIM    UINT16_C(0x0010)    ///< Obsolete. Must be zero
#define PE_HEADER_LARGE_ADDRESS_AWARE   UINT16_C(0x0020)    ///< Application can handle > 2-GB addresses.
#define PE_HEADER_BYTES_REVERSED_LO     UINT16_C(0x0080)    ///< Bytes of machine word are reversed.
#define PE_HEADER_32BIT_MACHINE         UINT16_C(0x0100)    ///< 32 bit word machine.
#define PE_HEADER_DEBUG_STRIPPED        UINT16_C(0x0200)    ///< Debugging info stripped from file in .DBG file.
#define PE_HEADER_SYSTEM                UINT16_C(0x1000)    ///< System File.
#define PE_HEADER_DLL                   UINT16_C(0x2000)    ///< File is a DLL.
#define PE_HEADER_UP_SYSTEM_ONLY        UINT16_C(0x4000)    ///< The file should be run only on a uniprocessor machine.
#define PE_HEADER_BYTES_REVERSED_HI     UINT16_C(0x8000)    ///< Bytes of machine word are reversed.

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
struct __packed PE_data_directory {
    uint32_t virtual_address;
    uint32_t size;
};

typedef struct PE_data_directory* PE_data_directory_t;

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
#define PE_HEADER_OPTIONAL_HDR64_MAGIC 0x20b

struct __packed PE_optional_header {
    uint16_t magic;
    /* Standard fields. */
    struct   PE_version8 linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    union {
        struct {
            uint32_t base_of_data;              ///< PE32 contains this additional field, which is absent in PE32+.
            uint32_t image_base32;
        };
        uint64_t image_base64;
    };
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
    union {
        struct {
            uint64_t size_of_stack_reserve32;
            uint64_t size_of_stack_commit32;
            uint64_t size_of_heap_reserve32;
            uint64_t size_of_heap_commit32;
            uint32_t loader_flags32;
            uint32_t number_of_RVA_and_sizes32;
            struct   PE_data_directory data_directory32[PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES];
        };
        struct {
            uint64_t size_of_stack_reserve64;
            uint64_t size_of_stack_commit64;
            uint64_t size_of_heap_reserve64;
            uint64_t size_of_heap_commit64;
            uint32_t loader_flags64;
            uint32_t number_of_RVA_and_sizes64;
            struct   PE_data_directory data_directory64[PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES];
        };
    };
};

#define PE_HEADER_SUBSYSTEM_UNKNOWN                     UINT16_C(0)  ///< An unknown subsystem
#define PE_HEADER_SUBSYSTEM_NATIVE                      UINT16_C(1)  ///< Device drivers and native Windows processes
#define PE_HEADER_SUBSYSTEM_WINDOWS_GUI                 UINT16_C(2)  ///< The Windows graphical user interface (GUI) subsystem
#define PE_HEADER_SUBSYSTEM_WINDOWS_CUI                 UINT16_C(3)  ///< The Windows character subsystem
#define PE_HEADER_SUBSYSTEM_OS2_CUI                     UINT16_C(5)  ///< The OS/2 character subsystem
#define PE_HEADER_SUBSYSTEM_POSIX_CUI                   UINT16_C(7)  ///< The Posix character subsystem
#define PE_HEADER_SUBSYSTEM_NATIVE_WINDOWS              UINT16_C(8)  ///< Native Win9x driver
#define PE_HEADER_SUBSYSTEM_WINDOWS_CE_GUI              UINT16_C(9)  ///< Windows CE
#define PE_HEADER_SUBSYSTEM_EFI_APPLICATION             UINT16_C(10) ///< An Extensible Firmware Interface (EFI) application
#define PE_HEADER_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER     UINT16_C(11) ///< An EFI driver with boot services
#define PE_HEADER_SUBSYSTEM_EFI_RUNTIME_DRIVER          UINT16_C(12) ///< An EFI driver with run-time services
#define PE_HEADER_SUBSYSTEM_EFI_ROM                     UINT16_C(13) ///< An EFI ROM image
#define PE_HEADER_SUBSYSTEM_XBOX                        UINT16_C(14) ///< XBOX
#define PE_HEADER_SUBSYSTEM_WINDOWS_BOOT_APPLICATION    UINT16_C(16) ///< Windows boot application.

#define PE_HEADER_DLLCHARACTERISTICS_HIGH_ENTROPY_VA    UINT16_C(0x0020) ///< Image can handle a high entropy 64-bit virtual address space.
#define PE_HEADER_DLLCHARACTERISTICS_DYNAMIC_BASE       UINT16_C(0x0040) ///< DLL can be relocated at load time.
#define PE_HEADER_DLLCHARACTERISTICS_FORCE_INTEGRITY    UINT16_C(0x0080) ///< Code Integrity checks are enforced.
#define PE_HEADER_DLLCHARACTERISTICS_NX_COMPAT          UINT16_C(0x0100) ///< Image is NX compatible.
#define PE_HEADER_DLLCHARACTERISTICS_NO_ISOLATION       UINT16_C(0x0200) ///< Isolation aware, but do not isolate the image.
#define PE_HEADER_DLLCHARACTERISTICS_NO_SEH             UINT16_C(0x0400) ///< Does not use structured exception (SE) handling. No SE handler may be called in this image.
#define PE_HEADER_DLLCHARACTERISTICS_NO_BIND            UINT16_C(0x0800) ///< Do not bind the image.
#define PE_HEADER_DLLCHARACTERISTICS_APPCONTAINER       UINT16_C(0x1000) ///< Image must execute in an AppContainer.
#define PE_HEADER_DLLCHARACTERISTICS_WDM_DRIVER         UINT16_C(0x2000)
#define PE_HEADER_DLLCHARACTERISTICS_GUARD_CF           UINT16_C(0x4000) ///< Image supports Control Flow Guard.
#define PE_HEADER_DLLCHARACTERISTICS_TS_AWARE           UINT16_C(0x8000) ///< Terminal Server aware.

#define PE_HEADER_SIGNATURE UINT32_C(0x00004550) /* PE\0\0 */

/**
 * @brief PE header
 */
struct __packed PE_image_headers {
    struct   PE_COFF_header file_header;
    struct   PE_optional_header optional_header;
};

typedef struct PE_image_headers* PE_image_headers_t;

/**
 * @brief Length of section name
 */
#define PE_SECTION_SIZE_OF_SHORT_NAME 8

/**
 * @brief Windows only alows 96 sections
 */
#define PE_HEADER_MAX_NUMBER_OF_SECTIONS 96

/**
 * @brief Section Table. This table immediately follows the optional header
 */
struct __packed PE_section_header {
    char name[PE_SECTION_SIZE_OF_SHORT_NAME];
    uint32_t virtual_size;
    uint32_t virtual_address;
    uint32_t size_of_raw_data;
    uint32_t pointer_to_raw_data;
    uint32_t pointer_to_relocations;
    uint32_t pointer_to_linenumbers;
    uint16_t number_of_relocations;
    uint16_t number_of_linenumbers;
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

#define PE_SECTION_ALIGN_MASK                0x00F00000

#define PE_SECTION_MEM_DISCARDABLE           0x02000000
#define PE_SECTION_MEM_NOT_CACHED            0x04000000
#define PE_SECTION_MEM_NOT_PAGED             0x08000000
#define PE_SECTION_MEM_SHARED                0x10000000
#define PE_SECTION_MEM_EXECUTE               0x20000000
#define PE_SECTION_MEM_READ                  0x40000000
#define PE_SECTION_MEM_WRITE                 0x80000000

/**
 * @brief Base relocation
 */
struct __packed PE_base_relocation {
    uint32_t virtual_address;
    uint32_t size_of_block;
    uint16_t fixup[];
};

typedef struct PE_base_relocation* PE_base_relocation_t;

#define PE_RELOC_BASED_FIXUP_MASK       UINT16_C(0x0fff)
#define PE_RELOC_BASED_TYPE_MASK        UINT16_C(0xf000)

/* Based relocation types. */

/**
 * @brief The relocation is ignored.
 */
#define PE_RELOC_BASED_ABSOLUTE         UINT16_C(0x0000)

/**
 * @brief The base relocation adds the high 16 bits of the difference to the
 * 16-bit field at offset. The 16-bit field represents the high value of a
 * 32-bit word.
 */
#define PE_RELOC_BASED_HIGH             UINT16_C(0x1000)

/**
 * @brief The base relocation adds the low 16 bits of the difference to the
 * 16-bit field at offset. The 16-bit field represents the low half of a
 * 32-bit word.
 */
#define PE_RELOC_BASED_LOW              UINT16_C(0x2000)

/**
 * @brief The base relocation applies all 32 bits of the difference to the
 * 32-bit field at offset.
 */
#define PE_RELOC_BASED_HIGHLOW          UINT16_C(0x3000)

/**
 * @brief The base relocation adds the high 16 bits of the difference to the
 * 16-bit field at offset. The 16-bit field represents the high value of a
 * 32-bit word. The low 16 bits of the 32-bit value are stored in the 16-bit
 * word that follows this base relocation. This means that this base
 * relocation occupies two slots.
 *
 */
#define PE_RELOC_BASED_HIGHADJ          UINT16_C(0x4000)

/**
 * @brief The relocation interpretation is dependent on the machine type.
 * When the machine type is MIPS, the base relocation applies to a MIPS jump
 * instruction.
 */
#define PE_RELOC_BASED_MIPS_JMPADDR     UINT16_C(0x5000)

/**
 * @brief This relocation is meaningful only when the machine type is ARM or
 * Thumb. The base relocation applies the 32-bit address of a symbol across a
 * consecutive MOVW/MOVT instruction pair.
 */
#define PE_RELOC_BASED_ARM_MOV32A       UINT16_C(0x5000)

/**
 * @brief This relocation is only meaningful when the machine type is RISC-V.
 * The base relocation applies to the high 20 bits of a 32-bit absolute
 * address.
 */
#define PE_RELOC_BASED_RISCV_HIGH20     UINT16_C(0x5000)

/**
 * @brief This relocation is meaningful only when the machine type is Thumb.
 * The base relocation applies the 32-bit address of a symbol to a consecutive
 * MOVW/MOVT instruction pair.
 */
#define PE_RELOC_BASED_THUMB_MOV32      UINT16_C(0x7000)

/**
 * @brief This relocation is only meaningful when the machine type is RISC-V.
 * The base relocation applies to the low 12 bits of a 32-bit absolute address
 * formed in RISC-V I-type instruction format.
 */
#define PE_RELOC_BASED_RISCV_LOW12I     UINT16_C(0x7000)

/**
 * @brief This relocation is only meaningful when the machine type is RISC-V.
 * The base relocation applies to the low 12 bits of a 32-bit absolute address
 * formed in RISC-V S-type instruction format.
 */
#define PE_RELOC_BASED_RISCV_LOW12S     UINT16_C(0x8000)

/**
 * @brief The relocation is only meaningful when the machine type is MIPS.
 * The base relocation applies to a MIPS16 jump instruction.
 */
#define PE_RELOC_BASED_MIPS_JMPADDR16   UINT16_C(0x9000)

/**
 * @brief The base relocation applies the difference to the 64-bit field at
 * offset.
 */
#define PE_RELOC_BASED_DIR64            UINT16_C(0xa000)
