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
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

/* default pagesize for EFI */
#define PAGE_SIZE 0x1000

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_VALUE(v, a) ((v) + (((a) - (v)) & ((a) - 1)))

#define PE_HEADER_SIZE 4098

static
struct section_vma {
    char name[PE_SECTION_SIZE_OF_SHORT_NAME + 1];
    char* filename;
    int fd;
    uint32_t target_vma;
    uint32_t virtual_size;
    uint32_t raw_address;
    uint32_t raw_size;
    uint32_t flags;
} section_data[] = {
    { .name = ".osrel",   .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".cmdline", .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".dtb",     .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".splash",  .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".linux",   .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { .name = ".initrd",  .target_vma = 0, .flags = PE_SECTION_CNT_INITIALIZED_DATA | PE_SECTION_MEM_READ },
    { }
};

enum section_data_id {
    SECTION_OSREL, SECTION_CMDLINE, SECTION_DT, SECTION_SPLASH, SECTION_LINUX, SECTION_INITRD, _SECTION_MAX
};

struct map {
    uint8_t* p;
    size_t size;
};

static inline
void close_p(int* fd) {
    if (*fd > 0)
        close(*fd);
    *fd = 0;
}

static inline
void unmap_p(struct map* map) {
    if (map->p)
        munmap(map->p, map->size);
    *map = (struct map) { };
}

static inline
void free_p(void **data) {
    if (*data)
        free(*data);
    *data = NULL;
}

static bool inspect_pe(
    int fd,
    size_t file_size,
    uint32_t* file_alignment,
    uint32_t* section_alignment,
    uint32_t* image_size,
    uint16_t* architecture
) {

    assert(filename);

    if (file_size < PE_HEADER_SIZE)
        return false;

    [[ gnu::cleanup(free_p) ]]
    void* base = malloc(PE_HEADER_SIZE);

    pread(fd, base, PE_HEADER_SIZE, 0);
    if (*(uint16_t*) base != MZ_DOS_SIGNATURE)
        return false;

    uint32_t offset = (*(uint32_t*) ((uint8_t*) base + DOS_PE_OFFSET_LOCATION));
    if (offset + sizeof(struct PE_image_headers) > PE_HEADER_SIZE)
        return false;

    PE_image_headers_t pe = (PE_image_headers_t) ((uint8_t*) base + offset);
    if (pe->file_header.signature != PE_HEADER_SIGNATURE)
        return false;

    if (pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR32_MAGIC
        && pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR64_MAGIC)
        return false;

    if (file_alignment)
        *file_alignment = pe->optional_header.file_alignment;
    if (section_alignment)
        *section_alignment = pe->optional_header.section_alignment;
    if (image_size)
        *image_size = pe->optional_header.size_of_image;
    if (architecture)
        *architecture = pe->file_header.machine;

    return true;
}

static
void usage() {
    printf("build_image [OPTIONS]\n"
        "\n"
        "\x1b[4mOptions:\x1b[0m\n"
        "  -h, --help         Show this help\n"
        "  -o, --outfile \x1b[3mPATH\x1b[0m Where the assembled boot file should be written to\n"
        "  -v, --verbose      Be more verbose\n"
        "  -s, --stub \x1b[3mPATH\x1b[0m    EFI stub\n"
        "  -l, --linux \x1b[3mPATH\x1b[0m   Linux kernel to embed\n"
        "  -i, --initrd \x1b[3mPATH\x1b[0m  initrd to embed\n"
        "  -d, --dtb \x1b[3mPATH\x1b[0m     DTB devicetree to embed\n"
        "  -c, --cmdline \x1b[3mPATH\x1b[0m cmdline to embed (textfile with single line)\n"
        "  -O, --osrel \x1b[3mPATH\x1b[0m   os-release file to embed (defaults to /etc/os-release)\n");
}

int main(int argc, char* argv[]) {
    struct PE_version16 efi_version = { 1, 10 };
    char* filename = NULL, *outfile = NULL;
    bool silent = true;

    const struct option long_opts[] = {
        { .name = "verbose",    .has_arg = no_argument,       .flag = NULL, .val = 'v' },
        { .name = "stub",       .has_arg = required_argument, .flag = NULL, .val = 's' },
        { .name = "outfile",    .has_arg = required_argument, .flag = NULL, .val = 'o' },
        { .name = "linux",      .has_arg = required_argument, .flag = NULL, .val = 'l' },
        { .name = "initrd",     .has_arg = required_argument, .flag = NULL, .val = 'i' },
        { .name = "dtb",        .has_arg = required_argument, .flag = NULL, .val = 'd' },
        { .name = "cmdline",    .has_arg = required_argument, .flag = NULL, .val = 'c' },
        { .name = "osrel",      .has_arg = required_argument, .flag = NULL, .val = 'O' },
        { .name = "efiversion", .has_arg = required_argument, .flag = NULL, .val = 'V' },
        { }
    };
    int c, opt_index = 0;

    while(-1 != (c = getopt_long(argc, argv, "sf:v:", long_opts, &opt_index))) {
        switch(c) {
            case 'v':
                silent = false;
                break;
            case 's':
                filename = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'O':
                section_data[SECTION_OSREL].filename = optarg;
                break;
            case 'c':
                section_data[SECTION_CMDLINE].filename = optarg;
                break;
            case 'd':
                section_data[SECTION_DT].filename = optarg;
                break;
            case 'l':
                section_data[SECTION_LINUX].filename = optarg;
                break;
            case 'i':
                section_data[SECTION_INITRD].filename = optarg;
                break;
            case 'V':
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
        fprintf(stderr, "Stub filename argument is required\n");
        usage();
        return 1;
    }

    if (!section_data[SECTION_LINUX].filename) {
        fprintf(stderr, "Kernel filename argument is required\n");
        usage();
        return 1;
    }

    if (!outfile) {
        fprintf(stderr, "Output filename argument is required\n");
        usage();
        return 1;
    }

    if (!section_data[SECTION_OSREL].filename) {
        section_data[SECTION_OSREL].filename = "/etc/os-release";
    }

    size_t orig_filesize = 0;
    uint32_t file_alignment;
    uint32_t section_alignment;
    uint16_t architecture;

    [[ gnu::cleanup(close_p) ]]
    int orig = openat(AT_FDCWD, filename, O_RDONLY);
    if (orig < 0) {
        fprintf(stderr, "open: '%s' %m\n", filename);
        return 1;
    }

    /* get the file size */
    struct statx st = { };
    if (0 > statx(orig, "", AT_EMPTY_PATH, STATX_SIZE, &st)) {
        fprintf(stderr, "stat: '%s' %m\n", filename);
        return 1;
    }

    orig_filesize = st.stx_size;

    if (!inspect_pe(orig, orig_filesize, &file_alignment, &section_alignment, NULL, &architecture)) {
        fprintf(stderr, "Invalid PE '%s'\n", filename);
        return 1;
    }

    if (section_alignment == 0)
        section_alignment = PAGE_SIZE;
    if (file_alignment == 0)
        file_alignment = 0x200;

    size_t filesize = orig_filesize;
    for (int i = 0; i < _SECTION_MAX; i++) {
        if (!section_data[i].filename)
            continue;
        section_data[i].fd = openat(AT_FDCWD, section_data[i].filename, O_RDONLY);
        if (section_data[i].fd < 0) {
            fprintf(stderr, "open: '%s' as '%s' %m\n", section_data[i].filename, section_data[i].name);
            return 1;
        }

        if (0 > statx(section_data[i].fd, "", AT_EMPTY_PATH, STATX_SIZE, &st)) {
            fprintf(stderr, "stat: '%s' %m\n", section_data[i].filename);
            return 1;
        }
        section_data[i].raw_size = st.stx_size;
        if (i == SECTION_LINUX) {
            uint32_t image_size; uint32_t linux_alignment; uint16_t linux_architecture;
            if (!inspect_pe(section_data[i].fd, st.stx_size, NULL, &linux_alignment, &image_size, &linux_architecture)) {
                fprintf(stderr, "Invalid PE '%s'\n", section_data[i].filename);
                return 1;
            }
            section_alignment = MAX(section_alignment, linux_alignment);
            section_data[i].virtual_size = ALIGN_VALUE(image_size, section_alignment);

            if (linux_architecture != architecture) {
                fprintf(stderr, "Linux '%s' and stub '%s' have different architectures\n", section_data[i].filename, filename);
                return 1;
            }
        }
        filesize += ALIGN_VALUE(st.stx_size, file_alignment);
    }

    [[ gnu::cleanup(close_p) ]]
    int fd = openat(AT_FDCWD, outfile, O_CREAT | O_EXCL | O_RDWR, 0644);
    if (fd < 0) {
        fprintf(stderr, "open: '%s' %m\n", outfile);
        return 1;
    }

    ssize_t ret = sendfile(fd, orig, NULL, orig_filesize);
    if (ret == -1) {
        fprintf(stderr, "sendfile: %m\n");
        return 1;
    }
    if (ret != orig_filesize) {
        fprintf(stderr, "sendfile: Data remaining: %zd != %zu\n", ret, orig_filesize);
        return 1;
    }

    // grow the file
    fsync(fd);
    ftruncate(fd, filesize);
    fsync(fd);

    assert(orig_filesize > 0);

    /* map PE file to memory to allow easier access */
    [[ gnu::cleanup(unmap_p) ]]
    struct map mem = {
        .p = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0),
        .size = filesize
    };
    if (mem.p == MAP_FAILED) {
        fprintf(stderr, "mmap: %m\n");
        return 1;
    }
    filesize = orig_filesize;

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

#define print_header_info(pe) \
    printf("target machine: %04hX subsystem: %hu (%hu.%hu) " \
        "flags: %04hX,%04hX version: %hu.%hu\n", \
        pe->file_header.machine, \
        pe->optional_header.subsystem, \
        pe->optional_header.subsystem_version.major, \
        pe->optional_header.subsystem_version.minor, \
        pe->file_header.characteristics, \
        pe->optional_header.DLL_characteristics, \
        pe->optional_header.image_version.major, \
        pe->optional_header.image_version.minor )

    if (pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR32_MAGIC
        && pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR64_MAGIC
    ) {
        fprintf(stderr, "missing OPTHDR signature\n");
        return 1;
    }
    if (pe->optional_header.subsystem == PE_HEADER_SUBSYSTEM_EFI_APPLICATION) {
        pe->optional_header.subsystem_version = efi_version;
        pe->optional_header.operation_system_version = efi_version;
        /* clear, this does not have any meaning to EFI */
        pe->optional_header.DLL_characteristics = 0;
    }

    pe->optional_header.file_alignment = file_alignment;
    pe->optional_header.section_alignment = section_alignment;

    /* print image information */
    if (!silent) {
        print_header_info(pe);
    }

    PE_section_t section = (PE_section_t)(
        (uint8_t*) pe + sizeof(struct PE_COFF_header)
        + pe->file_header.size_of_optional_header);

    /* find the end of the last section */
    size_t largest_raw_address = 0; size_t largest_vma = 0;
    for (uint16_t i = pe->file_header.number_of_sections; i--; section++) {
        if (largest_raw_address < section->pointer_to_raw_data + ALIGN_VALUE(section->pointer_to_raw_data + section->size_of_raw_data, file_alignment)) {
            largest_raw_address = ALIGN_VALUE(section->pointer_to_raw_data + section->size_of_raw_data, file_alignment);
        }
        if (largest_vma < section->virtual_address + section->virtual_size) {
            largest_vma = section->virtual_address + section->virtual_size;
        }
    }

    for (int i = 0; i < _SECTION_MAX; i++) {
        if (section_data[i].fd <= 0)
            continue;

        if (!silent)
            printf("put %8s at 0x%zx (%u)\n", section_data[i].name, largest_raw_address, section_data[i].raw_size);
        if (read(section_data[i].fd, base + largest_raw_address, section_data[i].raw_size) != section_data[i].raw_size) {
            fprintf(stderr, "read: '%s': %m\n", section_data[i].filename);
            return 1;
        }

        *section = (struct PE_section_header) {
            .virtual_size = ALIGN_VALUE(section_data[i].virtual_size == 0 ? section_data[i].raw_size : section_data[i].virtual_size, section_alignment),
            .virtual_address = section_data[i].target_vma == 0 ? largest_vma : section_data[i].target_vma ,
            .size_of_raw_data = section_data[i].raw_size,
            .pointer_to_raw_data = largest_raw_address,
            .characteristics = section_data[i].flags
        };
        strncpy(section->name, section_data[i].name, PE_SECTION_SIZE_OF_SHORT_NAME);

        pe->optional_header.size_of_initialized_data += section_data[i].raw_size;
        pe->optional_header.size_of_image += section->virtual_size;
        pe->file_header.number_of_sections++;
        largest_vma = section->virtual_address + section->virtual_size;
        section++;
        largest_raw_address = ALIGN_VALUE(largest_raw_address + section_data[i].raw_size, file_alignment);
        filesize += ALIGN_VALUE(section_data[i].raw_size, file_alignment);
        assert(filesize < mem.size);
    }

    if (!silent) {
        printf("vmasize: %08X section alignment: %u file alignment: %u\n",
            (uint32_t) pe->optional_header.size_of_image,
            pe->optional_header.section_alignment,
            pe->optional_header.file_alignment);
    }

    if (0 > msync(mem.p, mem.size, MS_SYNC)) {
        printf("msync %m");
    }

    ftruncate(fd, filesize);
}
