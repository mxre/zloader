#define _GNU_SOURCE /* for statx */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include "pe.h"

#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* default pagesize for EFI */
#define PAGE_SIZE 0x1000

struct map {
    uint8_t* p;
    size_t size;
};

static inline
void close_p(int* fd) {
    if (*fd > 0)
        close(*fd);
}

static inline
void unmap_p(struct map* map) {
    if (map->p)
        munmap(map->p, map->size);
}

#define ALIGN_VALUE(v, a) ((v) + (((a) - (v)) & ((a) - 1)))

static
void usage() {
    printf("pe_fixup --file <filename> [--efiversion XX.YY]\n"
        "\n"
        "    <filename> is an existing EFI executable\n"
        "    XX.YY a valid EFI version (like 2.31 or 2.70)\n"
        "    which is the minimum required EFI version for the executable\n");
}

int main(int argc, char* argv[]) {
    struct PE_version16 efi_version = { 1, 10 };
    char* filename = NULL;
    bool silent = false, set_version = false;

    const struct option long_opts[] = {
        { .name = "silent",     .has_arg = no_argument,       .flag = NULL, .val = 's' },
        { .name = "file",       .has_arg = required_argument, .flag = NULL, .val = 'f' },
        { .name = "efiversion", .has_arg = required_argument, .flag = NULL, .val = 'v' },
        { }
    };
    int c, opt_index = 0;

    while(-1 != (c = getopt_long(argc, argv, "sf:v:", long_opts, &opt_index))) {
        switch(c) {
            case 's':
                silent = true;
                break;
            case 'f':
                filename = optarg;
                break;
            case 'v':
                {
                    uint16_t major, minor;
                    if (2 != sscanf(optarg, "%hu.%hu", &major, &minor)) {
                        fprintf(stderr, "Could not parse version\n");
                        usage();
                        return 1;
                    }
                    if (minor > 99) {
                        fprintf(stderr, "Illegal EFI version\n");
                        usage();
                        return 1;
                    }
                    efi_version.major = major;
                    efi_version.minor = minor;
                    set_version = true;
                }
                break;
            case '?': /* unknown option */
                usage();
                return 1;
            default:
                assert(true);
        }
    }

    if (!filename) {
        fprintf(stderr, "Filename argument is required\n");
        usage();
        return 1;
    }

    [[ gnu::cleanup(close_p) ]]
    int fd = openat(AT_FDCWD, filename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open: %m\n");
        return 1;
    }

    /* get the file size */
    struct statx st = { };
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

    /*== PE header fixup ==*/

    /* clang and llvm do not expose any of these */
    pe->file_header.characteristics |=
        PE_HEADER_LINE_NUMS_STRIPPED
        | PE_HEADER_LOCAL_SYMS_STRIPPED
        | PE_HEADER_DEBUG_STRIPPED;

    uint32_t section_alignment; uint32_t file_alignment;
    if (pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR32_MAGIC
        && pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR64_MAGIC
    ) {
        fprintf(stderr, "missing OPTHDR signature\n");
        return 1;
    }
    if (pe->optional_header.subsystem == PE_HEADER_SUBSYSTEM_EFI_APPLICATION) {
        if (set_version) {
            pe->optional_header.subsystem_version = efi_version;
            pe->optional_header.operation_system_version = efi_version;
        }
        /* clear, this does not have any meaning to EFI */
        pe->optional_header.DLL_characteristics = 0;
    }

    if (pe->optional_header.file_alignment == 0) {
        fprintf(stderr, "file has file alginment header set to 0, overwriting with %u\n", 0x200);
        pe->optional_header.file_alignment = 0x200;
    }
    if (pe->optional_header.section_alignment == 0) {
        fprintf(stderr, "file has section alginment header set to 0, overwriting with %u\n", PAGE_SIZE);
        pe->optional_header.section_alignment = PAGE_SIZE;
    }

    section_alignment = pe->optional_header.section_alignment;
    file_alignment = pe->optional_header.file_alignment;

    /* print image information */
    if (!silent) {
        printf("target machine: %04hX subsystem: %hu (%hu.%hu) "
            "flags: %04hX,%04hX version: %hu.%hu\n",
            pe->file_header.machine,
            pe->optional_header.subsystem,
            pe->optional_header.subsystem_version.major,
            pe->optional_header.subsystem_version.minor,
            pe->file_header.characteristics,
            pe->optional_header.DLL_characteristics,
            pe->optional_header.image_version.major,
            pe->optional_header.image_version.minor
        );
    }

    /*== PE section VMA and flags ==*/

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

        if (!silent) {
            printf("%10.*s (%08x %10u) (%08x %12u) (%04x %4hu) %08x\n",
                PE_SECTION_SIZE_OF_SHORT_NAME, section->name,
                section->virtual_address, section->virtual_size,
                section->pointer_to_raw_data, section->size_of_raw_data,
                section->pointer_to_relocations, section->number_of_relocations,
                section->characteristics
            );
        }
    }

    pe->optional_header.size_of_image = largest_vma;
    if (!silent) {
        printf("vmasize: %08X section alignment: %u file alignment: %u\n",
            (uint32_t) pe->optional_header.size_of_image,
            pe->optional_header.section_alignment,
            pe->optional_header.file_alignment);
    }

    if (0 > msync(mem.p, mem.size, MS_SYNC)) {
        printf("msync %m");
    }
}
