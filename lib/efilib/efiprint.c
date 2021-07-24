/**
 * @file efiprint.c
 * @author Max Resch
 * @brief printf implementation based on GNU-EFI Print function
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 * @see https://github.com/rhboot/gnu-efi/blob/master/lib/print.c
 */
#include <efi.h>
#include <efilib.h>
#include <minmax.h>

#define PRINT_STRING_LEN            200
#define PRINT_ITEM_BUFFER_LEN       200

struct ptr {
    bool is_ascii;
    efi_size_t index;
    union {
        /* pointer for unicode */
        const char16_t* pu;

        /* pointer for ascii */
        const char8_t* pb;
    };
};

struct _pitem {
    struct ptr fmt;
    char16_t scratch[PRINT_ITEM_BUFFER_LEN];
    efi_size_t width;
    efi_size_t field_width;
    efi_size_t* width_parse;
    char16_t pad_char;
    bool pad_before;
    bool is_long;  /* is 64bit */ 
    bool is_short; /* is 16 bit */
};

typedef efi_api efi_size_t (*output_string)(void* context, const char16_t* str);
typedef efi_api efi_size_t (*output_setattr)(void* context, efi_size_t attr);

struct print_state {
    /* Input */
    struct ptr fmt;
    va_list args;

    /* Output */
    char16_t* buffer;
    char16_t* pos;
    char16_t* end;
    efi_size_t len;

    efi_size_t attr;
    efi_size_t restore_attr;

    efi_size_t attr_norm;
    efi_size_t attr_highlight;
    efi_size_t attr_error;
    bool add_cr;

    output_string output;
    output_setattr set_attr;
    void* context;

    /* Current item being formatted */
    struct _pitem  *item;
};

char16_t* value_to_string (
    char16_t* buffer,
    int64_t v
) {
    if (!v) {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer + 1;
    }

    char8_t str[30];
    char8_t* p1 = str;
    char16_t* p2 = buffer;

    if (v < 0) {
        *(p2++) = u'-';
        v = -v;
    }

    while (v) {
        lldiv_t r = lldiv((uint64_t) v, 10);
        *(p1++) = (char16_t)r.rem + '0';
        v = r.quot;
    }

    while (p1 != str) {
        *(p2++) = *(--p1);
    }
    *p2 = 0;

    return p2;
}

static char8_t hex[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

char16_t* value_to_hex_string(
    char16_t* buffer,
    uint64_t value
) {
    if (!value) {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer + 1;
    }

    char8_t str[30];
    char8_t* p1 = str;
    char16_t* p2 = buffer;

    while (value) {
        *(p1++) = hex[(efi_size_t) (value & 0xf)];
        value = value >> 4;
    }

    while (p1 != str) {
        *(p2++) = *(--p1);
    }
    *p2 = 0;

    return p2;
}

#if EFILIB_FLOATING_POINT
char16_t* float_to_string (
    char16_t* buffer,
    double value
) {
    /* Integer part */
    intptr_t i = (intptr_t) value;
    char16_t* p = value_to_string(buffer, i);

    /* Decimal point */
    *p = '.';
    p++;

    /* Keep fractional part */
    float f = (float) (value - i);
    if (f < 0) f = -f;

    /* Leading fractional zeroes */
    f *= 10.0;
    while ((f != 0) && ((intptr_t) f == 0)) {
      *p = '0';
      p++;
      f *= 10.0;
    }

    /* Fractional digits */
    while ((float)(intptr_t) f != f)  {
      f *= 10;
    }
    return value_to_string(p, (intptr_t) f);
}
#endif /* EFILIB_FLOATING_POINT */

char16_t* guid_to_string(
    char16_t* buffer,
    efi_guid_t guid
) {
    char16_t* p = buffer;
    for (uint8_t i = (sizeof(uint32_t)) * 2; i--;)
        if (guid->ms1 < 0x10 << (4*i)) (*p++) = '0';
    p = value_to_hex_string(p, guid->ms1);
    (*p++) = '-';
    for (uint8_t i = (sizeof(uint16_t)-1) * 2; i--;)
        if (guid->ms2 < 0x10 << (4*i)) (*p++) = '0';
    p = value_to_hex_string(p, guid->ms2);
    (*p++) = '-';
    for (uint8_t i = (sizeof(uint16_t)-1) * 2; i--;)
        if (guid->ms3 < 0x10 << (4*i)) (*p++) = '0';
    p = value_to_hex_string(p, guid->ms3);
    (*p++) = '-';
    for (uint8_t i = 0; i < 2; i++) {
        if (guid->ms4[i] < 0x10) (*p++) = '0';
        p = value_to_hex_string(p, guid->ms4[i]);
    }
    (*p++) = '-';
    for (uint8_t i = 2; i < 8; i++) {
        if (guid->ms4[i] < 0x10) (*p++) = '0';
        p = value_to_hex_string(p, guid->ms4[i]);
    }
    return p;
}

char16_t* time_to_string(
    char16_t* buffer,
    efi_time_t time
) {
    /* ISO 8601 */
    char16_t* p = buffer;
    if (time->year < 1000) (*p++) = u'0';
    if (time->year < 100) (*p++) = u'0';
    if (time->year < 10) (*p++) = u'0';
    p = value_to_string(p, time->year);
    (*p++) = u'-';
    if (time->month < 10) (*p++) = '0';
    p = value_to_string(p, time->month);
    (*p++) = u'-';
    if (time->day < 10) (*p++) = '0';
    p = value_to_string(p, time->day);
    (*p++) = u'T';
    if (time->hour < 10) (*p++) = '0';
    p = value_to_string(p, time->hour);
    (*p++) = u':';
    if (time->minute < 10) (*p++) = '0';
    p = value_to_string(p, time->minute);
    (*p++) = u':';
    if (time->second < 10) (*p++) = '0';
    return value_to_string(p, time->second);
}

static inline
char16_t* _device_path_memmap(char16_t* buffer, efi_memory_device_path_t dp) {
    efi_size_t len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"MemMap(%hu,0x%lx,0x%lx)", dp->memory_type, dp->start, dp->end);
    return buffer + len;
}

static inline
char16_t* _device_path_vendor(char16_t* buffer, efi_vendor_device_path_t dp) {
    efi_size_t len;
    switch (dp->hdr.type) {
        case HARDWARE_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"VenHw(%g)", &dp->guid);
            break;
        case MESSAGING_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"VenMedia(%g)", &dp->guid);
            break;
        case MEDIA_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"VenMedia(%g)", &dp->guid);
            break;
        default:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"Ven?(%g)", &dp->guid);
    }
    return buffer + len;
}

static inline
char16_t* _device_path_scsi(char16_t* buffer, efi_scsi_device_path_t dp) {
    return buffer + wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"Scsi(%hu,%hu)", dp->target, dp->lun);
}

static inline
char16_t* _device_path_usb(char16_t* buffer, efi_usb_device_path_t dp) {
    return buffer + wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"Usb(%hu,%hu)", dp->port, dp->endpoint);
}

static inline
char16_t* _device_path_sata(char16_t* buffer, efi_sata_device_path_t dp) {
    return buffer + wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"Sata(%hu,%hu,%hu)", dp->hba_port, dp->port_multiplier_number, dp->lun);
}

static inline
char16_t* _device_path_sd(char16_t* buffer, efi_sd_device_path_t dp) {
    return buffer + wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"SD(%hu)", dp->slot_number);
}

static inline
char16_t* _device_path_partition(char16_t* buffer, efi_partition_device_path_t dp) {
    efi_size_t len;
    switch (dp->signature_type) {
        case SIGNATURE_TYPE_GUID:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,GPT,%g)", dp->partition_number, (efi_guid_t) &dp->signature);
            break;
        case SIGNATURE_TYPE_MBR:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,MBR,0x%X)", dp->partition_number, *((uint32_t*) &dp->signature));
            break;
        default:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN - 1, u"HD(%hu,%hu,0)", dp->partition_number, dp->signature_type);
    }
    return buffer + len;
}

static inline
char16_t* _device_path_filepath(char16_t* buffer, efi_filepath_t dp) {
    efi_size_t len = MIN(
        (dp->hdr.length - sizeof(dp->hdr))/sizeof(char16_t),
        PRINT_ITEM_BUFFER_LEN - 1);
    return wcsncpy(buffer, dp->pathname, len);
}

static inline
char16_t* _device_path_unknown(char16_t* buffer, efi_device_path_t dp) {
    efi_size_t len;
    switch (dp->type) {
        case HARDWARE_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN -1, u"HardwarePath(%hu)", dp->subtype);
            break;
        case MESSAGING_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN -1, u"MsgPath(%hu)", dp->subtype);
            break;
        case MEDIA_DEVICE_PATH:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN -1, u"MediaPath(%hu)", dp->subtype);
            break;
        default:
            len = wsprintf(buffer, PRINT_ITEM_BUFFER_LEN -1, u"Path(%hu,%hu)", dp->type, dp->subtype);
    }
    return buffer + len;
}

static inline
char16_t* _device_instance_end(char16_t* buffer, efi_device_path_t dp) {
    *(buffer++) = u',';
    return buffer;
}

char16_t* device_path_to_string(
    char16_t* buffer,
    efi_device_path_t dp
) {
    EFILIB_ASSERT(dp);
    for(; !IsDevicePathEndNode(dp); dp = NextDevicePathNode(dp)) {
        EFILIB_ASSERT(dp->type != 0);
        if (dp->type == HARDWARE_DEVICE_PATH) {
            if (dp->subtype == HW_MEMMAP_DP)
                buffer = _device_path_memmap(buffer, (efi_memory_device_path_t) dp);
            else if (dp->subtype == HW_VENDOR_DP)
                buffer = _device_path_vendor(buffer, (efi_vendor_device_path_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == MEDIA_DEVICE_PATH) {
            if (dp->subtype == MEDIA_PARTITION_DP)
                buffer = _device_path_partition(buffer, (efi_partition_device_path_t) dp);
            else if (dp->subtype == MEDIA_VENDOR_DP)
                buffer = _device_path_vendor(buffer, (efi_vendor_device_path_t) dp);
            else if (dp->subtype == MEDIA_FILEPATH_DP)
                buffer = _device_path_filepath(buffer, (efi_filepath_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == MESSAGING_DEVICE_PATH) {
            if (dp->subtype == MSG_SCSI_DP) {
                buffer = _device_path_scsi(buffer, (efi_scsi_device_path_t) dp);
            } else if (dp->subtype == MSG_SATA_DP) {
                buffer = _device_path_sata(buffer, (efi_sata_device_path_t) dp);
            } else if (dp->subtype == MSG_USB_DP) {
                buffer = _device_path_usb(buffer, (efi_usb_device_path_t) dp);
            } else if (dp->subtype == MSG_SD_DP)
                buffer = _device_path_sd(buffer, (efi_sd_device_path_t) dp);
            else
                buffer = _device_path_unknown(buffer, dp);
        } else if (dp->type == END_DEVICE_PATH_TYPE) {
            if (dp->subtype == END_INSTANCE_DEVICE_PATH_SUBTYPE)
                buffer = _device_instance_end(buffer, dp);
        } else {
            buffer = _device_path_unknown(buffer, dp);
        }
        if (!IsDevicePathEndNode(NextDevicePathNode(dp)))
            *(buffer++) = u'/';
        *buffer = u'\0';
    }
    return buffer;
}

#if EFILIB_ERROR_MESSAGES
static struct {
    efi_status_t code;
    char8_t*    desc;
} error_codes[] = {
    {  EFI_SUCCESS,                "Success"},
    {  EFI_LOAD_ERROR,             "Load Error"},
    {  EFI_INVALID_PARAMETER,      "Invalid Parameter"},
    {  EFI_UNSUPPORTED,            "Unsupported"},
    {  EFI_BAD_BUFFER_SIZE,        "Bad Buffer Size"},
    {  EFI_BUFFER_TOO_SMALL,       "Buffer Too Small"},
    {  EFI_NOT_READY,              "Not Ready"},
    {  EFI_DEVICE_ERROR,           "Device Error"},
    {  EFI_WRITE_PROTECTED,        "Write Protected"},
    {  EFI_OUT_OF_RESOURCES,       "Out of Resources"},
    {  EFI_VOLUME_CORRUPTED,       "Volume Corrupt"},
    {  EFI_VOLUME_FULL,            "Volume Full"},
    {  EFI_NO_MEDIA,               "No Media"},
    {  EFI_MEDIA_CHANGED,          "Media changed"},
    {  EFI_NOT_FOUND,              "Not Found"},
    {  EFI_ACCESS_DENIED,          "Access Denied"},
    {  EFI_NO_RESPONSE,            "No Response"},
    {  EFI_NO_MAPPING,             "No mapping"},
    {  EFI_TIMEOUT,                "Time out"},
    {  EFI_NOT_STARTED,            "Not started"},
    {  EFI_ALREADY_STARTED,        "Already started"},
    {  EFI_ABORTED,                "Aborted"},
    {  EFI_ICMP_ERROR,             "ICMP Error"},
    {  EFI_TFTP_ERROR,             "TFTP Error"},
    {  EFI_PROTOCOL_ERROR,         "Protocol Error"},
    {  EFI_INCOMPATIBLE_VERSION,   "Incompatible Version"},
    {  EFI_SECURITY_VIOLATION,     "Security Policy Violation"},
    {  EFI_CRC_ERROR,              "CRC Error"},
    {  EFI_END_OF_MEDIA,           "End of Media"},
    {  EFI_END_OF_FILE,            "End of File"},
    {  EFI_INVALID_LANGUAGE,       "Invalid Languages"},
    {  EFI_COMPROMISED_DATA,       "Compromised Data"},

    // warnings
    {  EFI_WARN_UNKNOWN_GLYPH,     "Warning Unknown Glyph"},
    {  EFI_WARN_DELETE_FAILURE,    "Warning Delete Failure"},
    {  EFI_WARN_WRITE_FAILURE,     "Warning Write Failure"},
    {  EFI_WARN_BUFFER_TOO_SMALL,  "Warning Buffer Too Small"},
    {  0, NULL}
};
#endif /* EFILIB_ERROR_MESSAGES */

char16_t* status_to_string(
    char16_t* buffer,
    efi_status_t status
) {
#if EFILIB_ERROR_MESSAGES
    for (size_t i = 0; error_codes[i].desc; i +=1) {
        if (error_codes[i].code == status) {
            mbstowcs(buffer, error_codes[i].desc, strlen(error_codes[i].desc));
	        return buffer;
        }
    }
#endif /* EFILIB_ERROR_MESSAGES */
    char16_t* p = buffer;
    for (uint8_t i = (sizeof(efi_status_t) - 1) * 2; i--;)
    if (status < 0x10 << i) (*p++) = u'0';
    return value_to_hex_string(p, status);
}

static inline
void ptr_flush(
    struct print_state* ps
) {
    if (ps->output) {
        *ps->pos = u'\0';
        ps->output(ps->context, ps->buffer);
        ps->pos = ps->buffer;
    }
}

static inline
void ptr_setattr(
    struct print_state* ps,
    efi_size_t attr
)
{
   ptr_flush(ps);

   ps->restore_attr = ps->attr;
   if (ps->set_attr) {
       ps->set_attr(ps->context, attr);
   }

   ps->attr = attr;
}

static inline
void ptr_putc(
    struct print_state* ps,
    char16_t c
) {
    /* if this is a newline, add a carraige return */
    if (c == u'\n' && ps->add_cr) {
        ptr_putc(ps, u'\r');
    }

    *ps->pos = c;
    ps->pos++;
    ps->len++;

    /* if at the end of the buffer, flush it */
    if (ps->pos >= ps->end) {
        ptr_flush(ps);
    }
}

static inline
char16_t ptr_getc(
    struct ptr* p
) {
    char16_t c = u'?';
    if (p->is_ascii)  {
        int ret = mbtowc(&c, p->pb + p->index, 8);
        if (ret > 0)
            p->index += ret;
    } else c = p->pu[p->index++];
    return c;
}

static inline
void ptr_item(
    struct print_state *ps
) {
    /* Get the length of the item */
    struct _pitem *item = ps->item;
    item->fmt.index = 0;
    while (item->fmt.index < item->field_width) {
        char16_t c = ptr_getc(&item->fmt);
        if (!c) {
            item->fmt.index--;
            break;
        }
    }
    efi_size_t len = item->fmt.index;

    /* if there is no item field width, use the items width */
    if (item->field_width == (efi_size_t) -1) {
        item->field_width = len;
    }

    /* if item is larger then width, update width */
    if (len > item->width) {
        item->width = len;
    }

    /* if pad field before, add pad char */
    if (item->pad_before) {
        for (efi_size_t i = item->width; i < item->field_width; i+=1) {
            ptr_putc(ps, u' ');
        }
    }

    /* pad item */
    for (efi_size_t i = len; i < item->width; i++) {
        ptr_putc(ps, item->pad_char);
    }

    /* add the item */
    item->fmt.index = 0;
    while (item->fmt.index < len) {
        ptr_putc(ps, ptr_getc(&item->fmt));
    }

    /* If pad at the end, add pad char */
    if (!item->pad_before) {
        for (efi_size_t i = item->width; i < item->field_width; i+=1) {
            ptr_putc(ps, u' ');
        }
    }
}

efi_size_t _print(
    struct print_state* ps
) {
    EFILIB_ASSERT(ps);

    char16_t c;
    struct _pitem item;

    ps->len = 0;
    ps->item = &item;
    ps->fmt.index = 0;

    while ((c = ptr_getc(&ps->fmt))) {
        if (c != '%') {
            ptr_putc(ps, c);
            continue;
        }

        /* setup for new item */
        item.field_width = (efi_size_t) -1;
        item.width = 0;
        item.width_parse = &item.width;
        item.pad_char = u' ';
        item.pad_before = true;
        item.is_long = false;
        item.is_short = false;
        item.fmt.is_ascii = false;
        item.fmt.pb = NULL;
        item.fmt.pu = NULL;
        ps->restore_attr = 0;
        efi_size_t attr = 0;

        while ((c = ptr_getc(&ps->fmt))) {
            switch(c) {
                case '%':
                    item.scratch[0] = u'%';
                    item.scratch[1] = 0;
                    item.fmt.pu = item.scratch;
                    break;

                case '0':
                    item.pad_char = u'0';
                    break;

                case '-':
                    item.pad_before = false;
                    break;

                case '.':
                    item.width_parse = &item.field_width;
                    break;

                case '*':
                    *item.width_parse = va_arg(ps->args, efi_size_t);
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    *item.width_parse = 0;
                    do {
                        *item.width_parse = *item.width_parse * 10 + c - '0';
                        c = ptr_getc(&ps->fmt);
                    } while (c >= '0'  &&  c <= '9') ;
                    ps->fmt.index--;
                    break;

                case 's':
                    if (item.is_long) { 
                        item.fmt.pu = va_arg(ps->args, char16_t*);
                        if (!item.fmt.pu) {
                            item.fmt.pu = u"(null)";
                        }
                    } else {
                        item.fmt.pb = va_arg(ps->args, char8_t*);
                        item.fmt.is_ascii = true;
                        if (!item.fmt.pb) {
                            item.fmt.pb = (char8_t*) "(null)";
                        }
                    }
                    break;

                case 'c':
                    /* should work for 8bit chars too */ 
                    item.scratch[0] = (char16_t) va_arg(ps->args, efi_size_t);
                    item.scratch[1] = 0;
                    item.fmt.pu = item.scratch;
                    break;

                case 'h':
                    item.is_short = true;
                    break;
                
                case 'l':
                    item.is_long = true;
                    break;

                case 'z':
                    item.is_long = sizeof(efi_size_t) == 8;
                    break;

                case 'p':
                case 'X':
                    item.width =
                        c == 'p' ? sizeof(efi_size_t) * 2 :
                        item.is_short ? 4  : 
                        item.is_long  ? 16 : 8;
                    item.pad_char = u'0';
                    
                __fallthrough__;
                case 'x':
                    if (item.is_short) {
                        value_to_hex_string(
                            item.scratch,
                            (uint16_t) va_arg(ps->args, uint32_t)
                        );
                    } else {
                        value_to_hex_string(
                            item.scratch,
                            item.is_long ?
                                va_arg(ps->args, uint64_t) :
                                va_arg(ps->args, uint32_t));
                    }
                    item.fmt.pu = item.scratch;
                    break;

                case 'D':
                    device_path_to_string(item.scratch, va_arg(ps->args, efi_device_path_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'g':
                    guid_to_string(item.scratch, va_arg(ps->args, efi_guid_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'u':
                    if (item.is_short) {
                        value_to_string(
                            item.scratch,
                            (uint16_t) va_arg(ps->args, int)
                        );
                    } else {
                        value_to_string (
                            item.scratch,
                            item.is_long ?
                                va_arg(ps->args, uint64_t) :
                                va_arg(ps->args, uint32_t));
                    }
                    item.fmt.pu = item.scratch;
                    break;

                case 'i':
                case 'd':
                if (item.is_short) {
                        value_to_string (
                            item.scratch,
                            (uint16_t) va_arg(ps->args, int)
                        );
                    } else {
                        value_to_string (
                            item.scratch,
                            item.is_long ?
                                va_arg(ps->args, uint64_t) :
                                va_arg(ps->args, uint32_t));
                    }
                    item.fmt.pu = item.scratch;
                    break;

#if EFILIB_FLOATING_POINT
                case 'f':
                    float_to_string (
                        item.scratch,
                        va_arg(ps->args, double));
                    item.fmt.pu = item.scratch;
                    break;
#endif

                case 't':
                    time_to_string(item.scratch, va_arg(ps->args, efi_time_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'r':
                    status_to_string(item.scratch, va_arg(ps->args, efi_status_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'n':
                    ptr_setattr(ps, ps->attr_norm);
                    break;

                case 'b':
                    ptr_setattr(ps, ps->attr_highlight);
                    break;

                case 'e':
                    ptr_setattr(ps, ps->attr_error);
                    break;

                case 'N':
                    attr = ps->attr_norm;
                    break;

                case 'B':
                    attr = ps->attr_highlight;
                    break;

                case 'E':
                    attr = ps->attr_error;
                    break;

                default:
                    item.scratch[0] = u'?';
                    item.scratch[1] = 0;
                    item.fmt.pu = item.scratch;
                    break;
            }

            /* if we have an item */
            if (item.fmt.pu) {
                ptr_item(ps);
                break;
            }

            /* if we have an attr set */
            if (attr) {
                ptr_setattr(ps, attr);
                ps->restore_attr = 0;
                break;
            }
        }

        if (ps->restore_attr) {
            ptr_setattr(ps, ps->restore_attr);
        }
    }

    ptr_flush(ps);
    return ps->len;
}

efi_size_t _iprint(
    efi_size_t column,
    efi_size_t row,
    efi_simple_text_output_protocol_t out,
    const char16_t* fmt,
    const char8_t* fmta,
    va_list args
) {
    EFILIB_ASSERT(out != NULL);
    
    char16_t buffer[PRINT_STRING_LEN];
    struct print_state ps = { 0 };

    ps.context = out;
    ps.output = (output_string) out->output_string;
    ps.set_attr = (output_setattr) out->set_attribute;
    ps.add_cr = true;
    ps.buffer = buffer;
    ps.pos = buffer;
    ps.end = buffer + PRINT_STRING_LEN - 1;

    /* some UEFI implementation do not store/provide console attributes */
    ps.attr = out->mode->attribute
        ? out->mode->attribute
        : EFILIB_PRINT_NORMAL_COLOR | EFI_BACKGROUND_BLACK;

    ps.attr_norm = ps.attr & 0xff;
    ps.attr_highlight = EFILIB_PRINT_HIGHLIGHT_COLOR | (ps.attr & 0xf0);
    ps.attr_error = EFILIB_PRINT_ERROR_COLOR | (ps.attr & 0xf0);
    
    if (fmt) {
        ps.fmt.pu = fmt;
    } else if (fmta) {
        ps.fmt.is_ascii =true;
        ps.fmt.pb = fmta;
    } else
        return 0;

    if (column != (efi_size_t) -1)
        out->set_cursor_position(out, column, row);

    va_copy(ps.args, args);
    efi_size_t ret = _print(&ps);
    va_end(ps.args);
    return ret;
}

efi_size_t vswprintf(
    char16_t* restrict str,
    size_t maxlen,
    const char16_t* restrict fmt,
    va_list args
) {
    EFILIB_ASSERT(str != NULL);

    struct print_state ps = { 0 };
    ps.buffer = str;
    ps.pos = str;
    ps.end = str + maxlen - 1; 
    ps.fmt.pu = fmt;   

    va_copy(ps.args, args);
    efi_size_t ret = _print(&ps);
    *ps.pos = '\0';
    va_end(ps.args);
    return ret;
}
