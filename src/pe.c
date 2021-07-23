#include "pe.h"

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <efilib.h>

#include "util.h"

bool PE_locate_sections(
    PE_locate_sections_t locate_sections
) {
    assert(EFI_LOADED_IMAGE);

    if (!locate_sections)
        return false;

    struct simple_buffer image = {
        .buffer = EFI_LOADED_IMAGE->image_base,
        .pos = 0,
        .length = EFI_LOADED_IMAGE->image_size,
        .allocated = EFI_LOADED_IMAGE->image_size
    };

    size_t offset = PE_header(&image);
    if (!offset)
        return false;

    image.pos += offset;
    PE_image_headers_t pe = (PE_image_headers_t) buffer_pos(&image);
    if (pe->file_header.machine != PE_HEADER_MACHINE_NATIVE) {
        _MESSAGE("PE not for this platform: %hX", pe->file_header.machine);
        return false;
    }

    if (pe->file_header.number_of_sections > PE_HEADER_MAX_NUMBER_OF_SECTIONS){
        _MESSAGE("PE does contain too many sections: %hu", pe->file_header.number_of_sections);
        return false;
    }

    if ((pe->file_header.characteristics & PE_HEADER_EXECUTABLE_IMAGE) == 0){
        _MESSAGE("PE is not an executable: %hX", pe->file_header.characteristics);
        return false;
    }

#if __SIZE_WIDTH__ == 64   /* support PE32+ 64bit images */
    if (pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR64_MAGIC) {
        _MESSAGE("PE invalid OPT_HEADER magic: %hX", pe->optional_header.magic);
        return false;
    }

    PE_optional_header64_t opt_header = &pe->optional_header64;
#elif __SIZE_WIDTH__ == 32 /* support PE32 32bit images */
    if (pe->optional_header.magic != PE_HEADER_OPTIONAL_HDR32_MAGIC) {
        _MESSAGE("PE invalid OPT_HEADER magic: %hX", pe->optional_header.magic);
        return false;
    }

    PE_optional_header32_t opt_header = &pe->optional_header32;
#else
#  error "Only 32 and 64 bit platforms are supported"
#endif

    if (opt_header->subsystem != PE_HEADER_SUBSYSTEM_EFI_APPLICATION) {
        _MESSAGE("PE not an EFI application: %hX", opt_header->subsystem);
        return false;
    }

    image.pos += sizeof(struct PE_COFF_header)
        + pe->file_header.size_of_optional_header;

    /**
     * @see https://github.com/systemd/systemd/blob/main/src/boot/efi/pe.c
     */
    for (uint16_t i = pe->file_header.number_of_sections; i--; image.pos += sizeof(struct PE_section_header)) {
        PE_section_t sec = (PE_section_t) buffer_pos(&image);

        for (PE_locate_sections_t ls = locate_sections; *ls->name; ls++) {
            if (0 != strncmp(ls->name, sec->name, PE_SECTION_SIZE_OF_SHORT_NAME)) {
                continue;
            }

            ls->load_address = sec->virtual_address;
            ls->offset = sec->pointer_to_raw_data;
            ls->size = sec->virtual_size;
        }
        
    }

    return true;
}
