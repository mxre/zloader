#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include "pe.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define PAGE_SIZE 0x1000 

static struct PE_version16 efi_version = {
    .major = 2,
    .minor = 0
};

static struct section_vma {
    char name[PE_SECTION_SIZE_OF_SHORT_NAME];
    uint32_t target_vma;
    uint32_t flags;
} section_vma[] = {
    { .name = ".osrel",     .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".cmdline",   .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".linux",     .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".initrd",    .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { 0 }
};

struct map {
    uint8_t* p;
    size_t size;
};

static inline void close_p(int* fd) {
    if (*fd > 0)
        close(*fd);
}

static inline void unmap_p(struct map* map) {
    if (map->p)
        munmap(map->p, map->size);
}

#define ALIGN_VALUE(v, a) ((v) + (((a) - (v)) & ((a) - 1)))

#define __join(a,b) a ## b

#define print_header_info(pe, bits) \
    printf("target machine: %04hX (%u-bit) subsystem: %hu (%hu.%hu) " \
        "flags: %04hX,%04hX version: %hu.%hu\n", \
        pe->file_header.machine, \
        bits, \
        __join(pe->optional_header,bits).subsystem, \
        __join(pe->optional_header,bits).subsystem_version.major, \
        __join(pe->optional_header,bits).subsystem_version.minor, \
        pe->file_header.characteristics, \
        __join(pe->optional_header,bits).DLL_characteristics, \
        __join(pe->optional_header,bits).image_version.major, \
        __join(pe->optional_header,bits).image_version.minor )

#define print_alignment_header_info(pe, bits) \
    printf("base: %016lX vmasize: %08X section alignment: %u file alignment: %u\n", \
        (uint64_t) __join(pe->optional_header,bits).image_base, \
        (uint32_t) __join(pe->optional_header,bits).size_of_image, \
        __join(pe->optional_header,bits).section_alignment, \
        __join(pe->optional_header,bits).file_alignment )

/* https://github.com/rhboot/shim/blob/main/pe.c suggests that this is a possibility (althoug llvm does this right)*/
#define fix_alignment_header(pe, bits) \
    if ( __join(pe->optional_header,bits).file_alignment == 0) { \
        fprintf(stderr, "file has file alginment header set to 0, overwriting with %u\n", 0x200); \
         __join(pe->optional_header,bits).file_alignment = 0x200; \
    } \
    if ( __join(pe->optional_header,bits).section_alignment == 0) { \
        fprintf(stderr, "file has section alginment header set to 0, overwriting with %u\n", PAGE_SIZE); \
        __join(pe->optional_header,bits).section_alignment = PAGE_SIZE; \
    }

#define is_efi_app(pe, bits) \
    __join(pe->optional_header,bits).subsystem == PE_HEADER_SUBSYSTEM_EFI_APPLICATION
#define operation_system_version(pe, bits) \
    __join(pe->optional_header,bits).operation_system_version
#define subsystem_version(pe, bits) \
    __join(pe->optional_header,bits).subsystem_version

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "need file argument\n");
        return 1;
    }

    const char* filename = argv[1];

    [[ gnu::cleanup(close_p) ]]
    int fd = openat(AT_FDCWD, filename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open: %m\n");
        return 1;
    }

    /* get the file size */
    struct statx st = { 0 };
    if (0 > statx(fd, "", AT_EMPTY_PATH, STATX_SIZE, &st)) {
        fprintf(stderr, "stat: %m\n");
        return 1;
    }

    /* map PE file to memory to allow easier access */
    [[ gnu::cleanup(unmap_p) ]]
    struct map mem = {
        .p = mmap(NULL, st.stx_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0),
        .size = st.stx_size
    };
    if (mem.p == MAP_FAILED) {
        fprintf(stderr, "mmap: %m\n");
        return 1;
    }

    uint8_t* base = mem.p;

    /* check the file signature an file size */
    if (*(uint16_t*) base != MZ_DOS_SIGNATURE) {
        fprintf(stderr, "missing MZ signature\n");
        return 1;
    }

    if (mem.size < 64) {
        fprintf(stderr, "file too small\n");
        return 1;
    }

    uint32_t pe_offset = (*(uint32_t*) (base + DOS_PE_OFFSET_LOCATION));
    if (mem.size < pe_offset + 26) {
        fprintf(stderr, "file too small\n");
        return 1;
    }

    PE_image_headers_t pe = (PE_image_headers_t) (base + pe_offset);
    if (pe->file_header.signature != PE_HEADER_SIGNATURE) {
        fprintf(stderr, "missing PE signature\n");
        return 1;
    }

    if (pe->file_header.number_of_sections > PE_HEADER_MAX_NUMBER_OF_SECTIONS) {
        fprintf(stderr, "too many sections\n");
        return 1;
    }

    /* assure executable */
    if ((pe->file_header.characteristics & PE_HEADER_RELOCS_STRIPPED) != 0) {
        fprintf(stderr, "file marked as stripped from relocation table\n");
        return 1;
    }

    /* don't know how to handle this */
    if ((pe->file_header.characteristics & (PE_HEADER_BYTES_REVERSED_LO | PE_HEADER_BYTES_REVERSED_HI)) != 0) {
        fprintf(stderr, "file bit reversed\n");
        return 1;
    }

    if (mem.size < pe_offset + sizeof(struct PE_COFF_header) + pe->file_header.size_of_optional_header) {
        fprintf(stderr, "file to small\n");
        return 1;
    }

    /* clang and llvm do not expose any of these */
    pe->file_header.characteristics |= PE_HEADER_LINE_NUMS_STRIPPED
        | PE_HEADER_LOCAL_SYMS_STRIPPED
        | PE_HEADER_DEBUG_STRIPPED;

    uint32_t section_alignment; uint32_t file_alignment;
    if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        if (is_efi_app(pe, 32)) {
            subsystem_version(pe, 32) = efi_version;
            operation_system_version(pe, 32) = efi_version;
        }
 
        fix_alignment_header(pe, 32);
        section_alignment = pe->optional_header32.section_alignment;
        file_alignment = pe->optional_header32.file_alignment;
        /* print image information */
        print_header_info(pe, 32);
    } else if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR64_MAGIC) {
        if (is_efi_app(pe, 64)) {
            subsystem_version(pe, 64) = efi_version;
            operation_system_version(pe, 64) = efi_version;
        }

        fix_alignment_header(pe, 64);
        section_alignment = pe->optional_header64.section_alignment;
        file_alignment = pe->optional_header64.file_alignment;
        print_header_info(pe, 64);
    } else {
        fprintf(stderr, "missing OPTHDR signature\n");
        return 1;
    }

    PE_section_t section = (PE_section_t)(
        (uint8_t*) pe + sizeof(struct PE_COFF_header)
        + pe->file_header.size_of_optional_header);
    
    /* find the end of the vma */
    size_t largest_vma = 0;
    for (uint16_t i = pe->file_header.number_of_sections; i--; section++) {
        if (largest_vma < section->virtual_address) {
            largest_vma = section->virtual_address + section->virtual_size;
        }
    }
    largest_vma = ALIGN_VALUE(largest_vma, section_alignment);

    section = (PE_section_t)(
        (uint8_t*) pe + sizeof(struct PE_COFF_header)
        + pe->file_header.size_of_optional_header);
    for (uint16_t i = pe->file_header.number_of_sections; i--; section++) {
        if (section->size_of_raw_data == 0) {
            fprintf(stderr, "section %.*s has size 0 and is and is empty", PE_SECTION_SIZE_OF_SHORT_NAME, section->name);
            return 1;
        }
        for (struct section_vma* s = section_vma; *s->name; s++) {
            if (strncmp(s->name, section->name, PE_SECTION_SIZE_OF_SHORT_NAME) == 0) {
                /* only fix, if there is something to fix */
                if (section->virtual_address == 0) {
                    section->virtual_address = largest_vma;
                    /* llvm-objcopy uses alignment 1 */
                    if (section->virtual_size == 0)
                        section->virtual_size = section->size_of_raw_data,
                    section->characteristics = s->flags;

                    /* fix gaps in section table */
                    section->size_of_raw_data = ALIGN_VALUE(section->pointer_to_raw_data + section->size_of_raw_data, file_alignment) - section->pointer_to_raw_data;
                }    
            }
        }

        largest_vma = ALIGN_VALUE(section->virtual_address + section->virtual_size, section_alignment);

        printf("%10.*s (%08x %10u) (%08x %12u) (%04x %4hu) %08x\n",
            PE_SECTION_SIZE_OF_SHORT_NAME, section->name,
            section->virtual_address, section->virtual_size,
            section->pointer_to_raw_data, section->size_of_raw_data,
            section->pointer_to_relocations, section->number_of_relocations,
            section->characteristics
        );
    }

    if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        pe->optional_header32.size_of_image = largest_vma;
        print_alignment_header_info(pe, 32);
    } else if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR64_MAGIC) {
        pe->optional_header64.size_of_image = largest_vma;
        print_alignment_header_info(pe, 64);
    }

    if (0 > msync(mem.p, mem.size, MS_SYNC)) {
        printf("msync %m");
    }
}
