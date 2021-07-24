/**
 * @file pe_loader.c
 * @author Max Resch
 * @brief parse, verify and relocate PE executable for EFI
 * @version 0.1
 * @date 2021-07-20
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see https://github.com/rhboot/shim/blob/main/pe.c
 */
#include "pe.h"

#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "minmax.h"
#include "config.h"

struct pe_loader_ctx {
    efi_physical_address_t image_address;
    efi_size_t size_of_image;
    efi_virtual_address_t entry_point;
    efi_size_t size_of_headers;
    uint16_t number_of_sections;
    uint32_t section_alignment;
    uint32_t number_of_RVA_and_sizes;
    PE_section_t first_section; 
    PE_data_directory_t reloc_directory;
    PE_data_directory_t res_directory;
    PE_image_headers_t pe;                  ///< pointer to header struct
    uint8_t* base;                          ///< begin of image file
};

typedef struct pe_loader_ctx* pe_loader_ctx_t;

__constexp
static inline
bool allow_32bit() {
#if defined(__i386__) || defined(__arm__) || (defined(__riscv) && __riscv_xlen == 32)
    return true;
#else
    return false;
#endif
}

__constexp
static inline
bool allow_64bit() {
#if defined(__x86_64__) || defined(__aarch64__) || (defined(__riscv) && __riscv_xlen == 64)
    return true;
#else
    return false;
#endif
}

#ifndef __join
#  define __join(a,b) a ## b
#endif

#define ALIGN_VALUE(v, a) ((v) + (((a) - (v)) & ((a) - 1)))

/**
 * @brief transform image address to absolute address
 *  and perform bound checks
 * 
 * @param image
 *  pointer to image base address
 * @param size
 *  size of image
 * @param address
 *  address to translate
 * @return void*
 *  `image + address`, if in bounds or NULL
 */
__pure
static inline 
void* image_address (const void *image, uint64_t size, uint64_t address) {
    /* address outside of image */
	if (address > size)
		return NULL;

    /* address overflow bound check */
	if (UINT64_MAX - address < (uint64_t)(uintptr_t) image)
		return NULL;

	/* return pointer in address space */
	return (uint8_t*) image + address;
}

/**
 * @brief check if this file can be executed on the compiled platform
 * 
 * @details
 *  this checks if the supplied header has:
 *      * same machine type as this file
 *      * executable and/or library bit set in header
 *      * relocation information bit set in header
 *      * 32- or 64-bit magic WORD set
 *      * correct bitness
 *      * currently only allow for little endian platform
 */
static inline
efi_status_t is_loadable(
    PE_image_headers_t pe
) {
    assert(pe);

    /* I did not test bigendian so better just disallow it */
    if ((pe->file_header.characteristics & (PE_HEADER_BYTES_REVERSED_LO | PE_HEADER_BYTES_REVERSED_HI))) {
        _MESSAGE("Image unsupported - Image is BigEndian");
        return EFI_UNSUPPORTED;
    }

    if (pe->file_header.machine != PE_HEADER_MACHINE_NATIVE) {
        _MESSAGE("Image unsupported - Machine type for this platform %hX != %hX", pe->file_header.machine, PE_HEADER_MACHINE_NATIVE);
        return EFI_UNSUPPORTED;
    }

    if (!(pe->file_header.characteristics & (PE_HEADER_EXECUTABLE_IMAGE | PE_HEADER_DLL))) {
        _MESSAGE("Image unsupported - not a loadable PE file");
        return EFI_UNSUPPORTED;
    }

    /* relocation information is necessary */
    if ((pe->file_header.characteristics & PE_HEADER_RELOCS_STRIPPED)) {
        _MESSAGE("Image unsupported - stripped of relocations");
        return EFI_UNSUPPORTED;
    }

    if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        if (!allow_32bit()) {
            _MESSAGE("32-bit executable not supported");
            return EFI_UNSUPPORTED;
        }
    } else if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR64_MAGIC) {
        if (!allow_64bit()) {
            _MESSAGE("64-bit executable not supported");
            return EFI_UNSUPPORTED;
        }
    } else {
        _MESSAGE("Invalid OPTHDR magic %hX", pe->optional_header.magic);
        return EFI_UNSUPPORTED;
    }

    return EFI_SUCCESS;
}

static inline
efi_status_t read_headers(
    simple_buffer_t buffer,
    pe_loader_ctx_t ctx
) {
    assert(buffer);
    assert(buffer->buffer);
    assert(ctx);

    efi_status_t err;
    /* check for file signatures and get offset to headers */
    efi_size_t pe_offset = PE_header(buffer);
    if (!pe_offset) {
        _ERROR("Invalid PE image");
        return EFI_LOAD_ERROR;
    }
    
    uint8_t* image_base = (uint8_t*) buffer_pos(buffer);
    PE_image_headers_t pe = (PE_image_headers_t) (image_base + pe_offset);
    err = is_loadable(pe);
    if (EFI_ERROR(err)) {
        _ERROR("Platform does not support this image");
        return EFI_UNSUPPORTED;
    }

    uint32_t file_alignment, size_of_optheader;

#   define set_context(ctx, pe, bits) { \
    ctx->image_address = __join(pe->optional_header,bits).image_base; \
    ctx->number_of_RVA_and_sizes = __join(pe->optional_header,bits).number_of_RVA_and_sizes; \
    ctx->size_of_headers = __join(pe->optional_header,bits).size_of_headers; \
    ctx->size_of_image = __join(pe->optional_header,bits).size_of_image; \
    ctx->section_alignment = __join(pe->optional_header,bits).section_alignment; \
    size_of_optheader = sizeof(__join(pe->optional_header,bits)); \
    file_alignment =  __join(pe->optional_header,bits).file_alignment; }

    ctx->number_of_sections = pe->file_header.number_of_sections;
    ctx->entry_point = pe->optional_header.address_of_entry_point;
    ctx->base = image_base;

    if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        set_context(ctx, pe, 32);
    } else {
        set_context(ctx, pe, 64);
    }

#   undef set_context

    if (file_alignment % 2) {
        _MESSAGE("File Alignment is odd (%u)", file_alignment);
        return EFI_LOAD_ERROR;
    }
    if (file_alignment == 0) {
        file_alignment = 0x200;
        _MESSAGE("File alignment is 0, using %u instead", file_alignment);
    }
    if (ctx->section_alignment == 0) {
        ctx->section_alignment = MAX(file_alignment, PAGE_SIZE);
        _MESSAGE("Section alignment is 0, using %u instead", ctx->section_alignment);   
    }

    if (PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES < ctx->number_of_RVA_and_sizes) {
        _MESSAGE("RVA directory too large");
        return EFI_LOAD_ERROR;
    }

    {
        uint32_t header_sans_datadir = size_of_optheader
            - sizeof(struct PE_data_directory) * PE_HEADER_NUMBER_OF_DIRECTORY_ENTRIES;
        if (pe->file_header.size_of_optional_header - header_sans_datadir !=
            ctx->number_of_RVA_and_sizes * sizeof(struct PE_data_directory)
        ) {
            _MESSAGE("Image header overflows data directory");
            return EFI_LOAD_ERROR;
        }
    }

    uint32_t section_header_offset = pe_offset
        + sizeof(struct PE_COFF_header) 
        + pe->file_header.size_of_optional_header;
    if (((uint32_t) ctx->size_of_image - section_header_offset)
        / sizeof(struct PE_section_header) <= ctx->number_of_sections
    ) {
        _MESSAGE("Image section list overflow image size");
        return EFI_LOAD_ERROR;
    }

    if ((ctx->size_of_headers - section_header_offset)
        / sizeof(struct PE_section_header) < (uint32_t) ctx->number_of_sections
    ) {
        _MESSAGE("Image sections overflow section header");
        return EFI_LOAD_ERROR;
    }

    if (ctx->size_of_image < ctx->size_of_headers) {
		_MESSAGE("Header size exceeds image size\n");
		return EFI_LOAD_ERROR;
	}

    ctx->pe = pe;
    ctx->first_section = (PE_section_t) (image_base + section_header_offset);

#   define set_context(ctx, pe, bits) { \
    ctx->reloc_directory = &__join(pe->optional_header,bits).data_directory[PE_HEADER_DIRECTORY_ENTRY_BASERELOC]; \
    ctx->res_directory = &__join(pe->optional_header,bits).data_directory[PE_HEADER_DIRECTORY_ENTRY_RESOURCE]; }

    if (pe->optional_header.magic == PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        set_context(ctx, pe, 32);
    } else {
        set_context(ctx, pe, 64);
    }
#   undef set_context

    return EFI_SUCCESS;
}

/**
 * @brief Relocate section to virtual memory
 * 
 * @param buffer
 *  pointer to the base of the virtual memory segment 
 * @param ctx 
 * @return PE_section_t
 *  section header for the relocation section
 */
static inline
efi_status_t relocate_sections(
    const void* buffer,
    pe_loader_ctx_t ctx,
    PE_section_t* reloc_section
) {
    assert(buffer);
    assert(ctx);
    assert(ctx->first_section);
    assert(ctx->reloc_directory);
    assert(ctx->base && ctx->pe);
    assert(reloc_section);

    uint8_t* relocation_base = image_address(buffer, ctx->size_of_image,
        ctx->reloc_directory->virtual_address);
    uint8_t* relocation_end  = image_address(buffer, ctx->size_of_image,
        ctx->reloc_directory->virtual_address
        + ctx->reloc_directory->size + 1);

    PE_section_t sec = ctx->first_section;
    for (uint16_t i = ctx->number_of_sections; i--; sec++) {
        /* skip sections with size 0 marked discardable */
        if ((sec->characteristics & PE_SECTION_MEM_DISCARDABLE) && sec->virtual_size == 0)
            continue;
        
        uint8_t* base = image_address(buffer, ctx->size_of_image,
            sec->virtual_address);
        uint8_t* end =  image_address(buffer, ctx->size_of_image,
            sec->virtual_address + sec->virtual_size - 1);
        
        if (end < base) {
            _MESSAGE("Section %.*s has negative size", PE_SECTION_SIZE_OF_SHORT_NAME, sec->name);
            return EFI_LOAD_ERROR;
        }

        /* process .reloc even when marked discardable */
        if (strncmp(sec->name, ".reloc", PE_SECTION_SIZE_OF_SHORT_NAME) == 0) {
            if (*reloc_section) {
                _MESSAGE("Image has multiple .reloc sections");
                return EFI_LOAD_ERROR;
            }
            /* if this section appears valid than use it as relocation section */
            if (sec->size_of_raw_data && sec->virtual_size
                && base && end
                && relocation_base == base && relocation_end == end
            ) {
                *reloc_section = sec;
            }
        }

        /* skip sections marked discardable */
        if (sec->characteristics & PE_SECTION_MEM_DISCARDABLE)
            continue;

        /* loadable sections with vma or vma size 0 are invalid */ 
        if (!base || !end) {
            _ERROR("Section %.*s has illegal VMA", PE_SECTION_SIZE_OF_SHORT_NAME, sec->name);
            return EFI_LOAD_ERROR;
        }

        if (!(sec->characteristics & PE_SECTION_CNT_UNINITIALIZED_DATA) &&
		    (sec->virtual_address < ctx->size_of_headers || sec->pointer_to_raw_data < ctx->size_of_headers)
        ) {
			_MESSAGE("Section %.*s is inside image headers", PE_SECTION_SIZE_OF_SHORT_NAME, sec->name);
            return EFI_LOAD_ERROR;
		}

        if (sec->characteristics & PE_SECTION_CNT_UNINITIALIZED_DATA) {
			memset(base, 0, sec->virtual_size);
		} else {
            if (sec->pointer_to_raw_data < ctx->size_of_headers) {
				_MESSAGE("Section %.*s is inside image headers", PE_SECTION_SIZE_OF_SHORT_NAME, sec->name);
                return EFI_LOAD_ERROR;
			}
            
			if (sec->size_of_raw_data > 0)
                memcpy(base, ctx->base + sec->pointer_to_raw_data, sec->size_of_raw_data);

			if (sec->size_of_raw_data < sec->virtual_size)
                memset(base + sec->size_of_raw_data, 0, sec->virtual_size - sec->size_of_raw_data);
        }
    }

    if (!*reloc_section) {
        _MESSAGE("Image has no .reloc Section");
    }
    return EFI_SUCCESS;
}

/**
 * @brief 
 * 
 * @param buffer
 *  pointer to the base of the virtual memory segment 
 * @param ctx 
 * @param reloc_section
 *  pointer to .reloc section header
 */
efi_status_t relocation_fixup(
    uint8_t* buffer,
    pe_loader_ctx_t ctx,
    PE_section_t reloc_section
) {
    assert(buffer);
    assert(ctx);
    assert(ctx->base);
    assert(ctx->reloc_directory);
    assert(reloc_section);

    uint8_t* reloc_base = image_address(ctx->base, ctx->size_of_image,
        reloc_section->pointer_to_raw_data);
    uint8_t* reloc_end  = image_address(ctx->base, ctx->size_of_image,
        reloc_section->pointer_to_raw_data + reloc_section->virtual_size);
    
    if (!reloc_base && !reloc_end)
        return EFI_SUCCESS;
    if (!reloc_base || !reloc_end)
        return EFI_UNSUPPORTED;

    efi_size_t adjust = (efi_physical_address_t) buffer - ctx->image_address;
    /* image loaded at prefered base address */
    if (!adjust)
        return EFI_SUCCESS;

    uint32_t n = 0;
    for (
        PE_base_relocation_t reloc = (PE_base_relocation_t) reloc_base;
        (uint8_t*) reloc < reloc_end;
        reloc = (PE_base_relocation_t) ((uint8_t*) reloc) + reloc->size_of_block + sizeof(struct PE_base_relocation)
    ) {
        if (reloc->size_of_block == 0) {
            _MESSAGE("Reloc %u block size 0 is invalid", n);
            return EFI_UNSUPPORTED;
        } else if (reloc->size_of_block > ctx->reloc_directory->size) {
            _MESSAGE("Reloc %u block size %hu greater than reloc dir size %hu", n, reloc->size_of_block, ctx->reloc_directory->size);
            return EFI_UNSUPPORTED;
        }

        uint8_t* fixup_base = image_address(ctx->base, ctx->size_of_image, reloc->virtual_address);
        if (!fixup_base) {
            _MESSAGE("Reloc %u invalid virtual address", n);
            return EFI_UNSUPPORTED;
        }
        for (uint32_t i = 0; i < reloc->size_of_block / sizeof(uint16_t); i++) {
            uint8_t* fixup = fixup_base + (reloc->fixup[i] & PE_RELOC_BASED_FIXUP_MASK);
            switch(reloc->fixup[i] & PE_RELOC_BASED_TYPE_MASK) {
                case PE_RELOC_BASED_ABSOLUTE:
                    break;
                case PE_RELOC_BASED_HIGH: /* add upper WORD of the 32bit address */
                    *((uint16_t*) fixup) = (*((uint16_t*) fixup) + (uint16_t) ((uint32_t) adjust >> 16));
                    break;
                case PE_RELOC_BASED_LOW: /* add lower WORD of 32bit address */
                    *((uint16_t*) fixup) = (*((uint16_t*) fixup) + (uint16_t) adjust);
                    break;
                case PE_RELOC_BASED_HIGHLOW: /* add WORD */
                    *((uint32_t*) fixup) = (*((uint32_t*) fixup) + (uint32_t) adjust);
                    break;
                case PE_RELOC_BASED_DIR64:
                    *((uint64_t*) fixup) = (*((uint64_t*) fixup) + (uint64_t) adjust);
                    break;
                default:
                    _MESSAGE("Reloc %u Unknown relocation %u", n, reloc->fixup[i] >> 12);
                    return EFI_UNSUPPORTED;
            }
        }
        n++;
    }

    return EFI_SUCCESS;
}

/* unload a image loaded by PE_handle_image */
efi_api static
efi_status_t __unload_pe_file(
    efi_handle_t image
) {
    assert(BS);

    efi_status_t err;
    efi_memory_device_path_t dp;
   
    err = BS->open_protocol(image, &efi_loaded_image_device_path_guid, (void*) &dp,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_EXCLUSIVE);
    if (EFI_ERROR(err)) {
        return EFI_UNSUPPORTED;
    }

    /* free memory pages and device path node */
    if(dp->hdr.type == HARDWARE_DEVICE_PATH && dp->hdr.subtype == HW_MEMMAP_DP) {
        BS->free_pages(dp->start, (dp->start - dp->end) / PAGE_SIZE);
    } else {
        /* No MEMMAP devicepath, not our handle? */
        BS->close_protocol(image, &efi_loaded_image_device_path_guid, EFI_IMAGE, NULL);
        return EFI_UNSUPPORTED;
    }

    efi_loaded_image_t lp;
    err = BS->open_protocol(image, &efi_loaded_image_protocol_guid, (void*) &lp,
        EFI_IMAGE, NULL, EFI_OPEN_PROTOCOL_EXCLUSIVE);
    if (EFI_ERROR(err)) {
        BS->close_protocol(image, &efi_loaded_image_device_path_guid, EFI_IMAGE, NULL);
        return EFI_UNSUPPORTED;
    }

    BS->close_protocol(image, &efi_loaded_image_protocol_guid, EFI_IMAGE, NULL);
    BS->close_protocol(image, &efi_loaded_image_device_path_guid, EFI_IMAGE, NULL);

    err = BS->uninstall_multiple_protocol_interfaces(
        image,
        &efi_loaded_image_protocol_guid, lp,
        &efi_loaded_image_device_path_guid, dp,
        NULL
    );
    if (err == EFI_SUCCESS) {
        free(lp);
        free(dp);
    }

    return err;
}

efi_status_t PE_handle_image(
    simple_buffer_t data,
    efi_handle_t* image,
    efi_loaded_image_t* loaded_image,
    efi_entry_point_t* entry_point
) {
    efi_status_t err;

    assert(EFI_IMAGE);
    assert(EFI_LOADED_IMAGE);

    if (!data || !image || !loaded_image || !entry_point)
        return EFI_INVALID_PARAMETER;
    if (!data->buffer || buffer_len(data) == 0 )
        return EFI_INVALID_PARAMETER;

    struct pe_loader_ctx ctx;

    /* get required header fields and directory pointers */
    err = read_headers(data, &ctx);
    if (EFI_ERROR(err)) {
        _ERROR("Failed to read PE header: %r", err);
        return err;
    }

    assert(ctx.section_alignment);

    /* align pages on page size */
    uint32_t alloc_size = ALIGN_VALUE(ctx.size_of_image + ctx.section_alignment, PAGE_SIZE);
    efi_size_t allocated_pages = alloc_size / PAGE_SIZE;

    efi_physical_address_t allocated_address;
    err = BS->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_CODE, allocated_pages, &allocated_address);
    if (EFI_ERROR(err)) {
        return EFI_OUT_OF_RESOURCES;
    }

    /* align buffer to section alignment */
    uint8_t* buffer = (void*) ALIGN_VALUE((uint64_t) allocated_address, ctx.section_alignment);
    memcpy(buffer, ctx.base, ctx.size_of_headers);
    
    *entry_point = (efi_entry_point_t) image_address(buffer, ctx.size_of_image, ctx.entry_point);
    if (!*entry_point) {
        _ERROR("Entry point is invalid");
        BS->free_pages(allocated_address, allocated_pages);
        return EFI_LOAD_ERROR;
    }

    PE_section_t reloc_section = NULL;
    err = relocate_sections(buffer, &ctx, &reloc_section);
    if (EFI_ERROR(err)) {
        BS->free_pages(allocated_address, allocated_pages);
        return EFI_LOAD_ERROR;   
    }

    if (ctx.number_of_RVA_and_sizes <= PE_HEADER_DIRECTORY_ENTRY_BASERELOC) {
        _MESSAGE("Image has no relocation entry");
        BS->free_pages(allocated_address, allocated_pages);
        return EFI_UNSUPPORTED;
    }

    if (ctx.reloc_directory->size && reloc_section) {
        err = relocation_fixup(buffer, &ctx, reloc_section);

        if (EFI_ERROR(err)) {
            _MESSAGE("Relocation failed: %r", err);
            BS->free_pages(allocated_address, allocated_pages);
            return err;
        }
    }

    /* create device path for memory mapped file */
    efi_device_path_t dp = create_memory_mapped_device_path(
        allocated_address,  allocated_pages * PAGE_SIZE, EFI_LOADER_CODE);

    if (!dp) {
        BS->free_pages(allocated_address, allocated_pages);
        return EFI_OUT_OF_RESOURCES;
    }

    *loaded_image = malloc(sizeof(struct efi_loaded_image_protocol));
    if (!*loaded_image) {
        free(dp);
        BS->free_pages(allocated_address, allocated_pages);
        return EFI_OUT_OF_RESOURCES;
    }

    /* setup image handle */
    struct efi_loaded_image_protocol _lp = {
        .revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION,
        .parent_handle = EFI_IMAGE,
        .system_table = ST,
        .device_handle = EFI_LOADED_IMAGE->device_handle,
        .file_path = dp,
        .reserved = NULL,
        .image_base = buffer,
        .image_size = ctx.size_of_image,
        .image_code_type = EFI_LOADER_CODE,
        .image_data_type = EFI_LOADER_CODE, /* everything is located in the same memory allocation*/
        .unload = __unload_pe_file
    };
    **loaded_image = _lp;

    *image = NULL;
    err = BS->install_multiple_protocol_interfaces(
        image,
        &efi_loaded_image_protocol_guid, (void*) *loaded_image,
        &efi_loaded_image_device_path_guid, (void*) dp,
        NULL
    );
    if (EFI_ERROR(err)) {
        _MESSAGE("Creating image handle failed");
        free(dp);
        free(*loaded_image);
        BS->free_pages(allocated_address, allocated_pages);
        return err;
    }
    
    return EFI_SUCCESS;
}
