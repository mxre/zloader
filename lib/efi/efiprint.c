#include <efi.h>
#include <efilib.h>

#if EFI_PRINTF

#define PRINT_STRING_LEN            200
#define PRINT_ITEM_BUFFER_LEN       100

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
    bool is_long;
};

typedef efi_size_t (*output_string)(void* context, const char16_t* str);
typedef efi_size_t (*output_setattr)(void* context, efi_size_t attr);

struct print_state {
    /* Input */
    struct ptr fmt;
    va_list args;

    /* Output */
    char16_t* buffer;
    char16_t* end;
    char16_t* pos;
    efi_size_t len;

    efi_size_t attr;
    efi_size_t restore_attr;

    efi_size_t attr_norm;
    efi_size_t attr_highlight;
    efi_size_t attr_error;

    output_string output;
    output_setattr set_attr;
    void* context;

    /* Current item being formatted */
    struct _pitem  *item;
};

static inline
__attribute__((pure))
uint64_t div64(uint64_t n, uint32_t d, uint64_t* r) {
    if (r)
        *r = n % d;
    return n / d;
}

char16_t* value_to_string (
    char16_t* buffer,
    int64_t v
) {
    if (!v) {
        buffer[0] = u'0';
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
        uint64_t r;
        v = (int64_t) div64((uint64_t) v, 10, &r);
        *(p1++) = (char16_t)r + u'0';
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
        buffer[0] = u'0';
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

#if EFI_FLOATING_POINT
char16_t* float_to_string (
    char16_t* buffer,
    double value
) {
    /* Integer part */
    intptr_t i = (intptr_t) value;
    char16_t* p = value_to_string(buffer, i);

    /* Decimal point */
    *p = u'.';
    p++;

    /* Keep fractional part */
    float f = (float) (value - i);
    if (f < 0) f = -f;

    /* Leading fractional zeroes */
    f *= 10.0;
    while ((f != 0) && ((intptr_t) f == 0)) {
      *p = u'0';
      p++;
      f *= 10.0;
    }

    /* Fractional digits */
    while ((float)(intptr_t) f != f)  {
      f *= 10;
    }
    return value_to_string(p, (intptr_t) f);
}
#endif /* EFI_FLOATING_POINT */

char16_t* guid_to_string(
    char16_t* buffer,
    efi_guid_t* guid
) {
    char16_t* p = buffer;
    for (uint8_t i = (sizeof(uint32_t) - 1) * 2; i--;)
        if (guid->ms1 < 0x10 << i) (*p++) = u'0';
    p = value_to_hex_string(p, guid->ms1);
    (*p++) = u'-';
    for (uint8_t i = (sizeof(uint16_t) - 1) * 2; i--;)
        if (guid->ms2 < 0x10 << i) (*p++) = u'0';
    p = value_to_hex_string(p, guid->ms2);
    (*p++) = u'-';
    for (uint8_t i = (sizeof(uint16_t) - 1) * 2; i--;)
        if (guid->ms3 < 0x10 << i) (*p++) = u'0';
    p = value_to_hex_string(p, guid->ms3);
    (*p++) = u'-';
    for(uint8_t i = 0; i < 8; i++) {
        if (guid->ms4[i] < 0x10) (*p++) = u'0';
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

#if EFI_ERROR_MESSAGES
static struct {
    efi_status_t code;
    char16_t*    desc;
} error_codes[] = {
	{  EFI_SUCCESS,                u"Success"},
	{  EFI_LOAD_ERROR,             u"Load Error"},
	{  EFI_INVALID_PARAMETER,      u"Invalid Parameter"},
	{  EFI_UNSUPPORTED,            u"Unsupported"},
	{  EFI_BAD_BUFFER_SIZE,        u"Bad Buffer Size"},
	{  EFI_BUFFER_TOO_SMALL,       u"Buffer Too Small"},
	{  EFI_NOT_READY,              u"Not Ready"},
	{  EFI_DEVICE_ERROR,           u"Device Error"},
	{  EFI_WRITE_PROTECTED,        u"Write Protected"},
	{  EFI_OUT_OF_RESOURCES,       u"Out of Resources"},
	{  EFI_VOLUME_CORRUPTED,       u"Volume Corrupt"},
	{  EFI_VOLUME_FULL,            u"Volume Full"},
	{  EFI_NO_MEDIA,               u"No Media"},
	{  EFI_MEDIA_CHANGED,          u"Media changed"},
	{  EFI_NOT_FOUND,              u"Not Found"},
	{  EFI_ACCESS_DENIED,          u"Access Denied"},
	{  EFI_NO_RESPONSE,            u"No Response"},
	{  EFI_NO_MAPPING,             u"No mapping"},
	{  EFI_TIMEOUT,                u"Time out"},
	{  EFI_NOT_STARTED,            u"Not started"},
	{  EFI_ALREADY_STARTED,        u"Already started"},
	{  EFI_ABORTED,                u"Aborted"},
	{  EFI_ICMP_ERROR,             u"ICMP Error"},
	{  EFI_TFTP_ERROR,             u"TFTP Error"},
	{  EFI_PROTOCOL_ERROR,         u"Protocol Error"},
	{  EFI_INCOMPATIBLE_VERSION,   u"Incompatible Version"},
	{  EFI_SECURITY_VIOLATION,     u"Security Policy Violation"},
	{  EFI_CRC_ERROR,              u"CRC Error"},
	{  EFI_END_OF_MEDIA,           u"End of Media"},
	{  EFI_END_OF_FILE,            u"End of File"},
	{  EFI_INVALID_LANGUAGE,       u"Invalid Languages"},
	{  EFI_COMPROMISED_DATA,       u"Compromised Data"},

	// warnings
	{  EFI_WARN_UNKNOWN_GLYPH,     u"Warning Unknown Glyph"},
	{  EFI_WARN_DELETE_FAILURE,    u"Warning Delete Failure"},
	{  EFI_WARN_WRITE_FAILURE,     u"Warning Write Failure"},
	{  EFI_WARN_BUFFER_TOO_SMALL,  u"Warning Buffer Too Small"},
	{  0, NULL}
};
#endif /* EFI_ERROR_MESSAGES */

char16_t* status_to_string(
    char16_t* buffer,
    efi_status_t status
) {
#if EFI_ERROR_MESSAGES
    for (size_t i = 0; error_codes[i].desc; i +=1) {
        if (error_codes[i].code == status) {
	        return strcpy(buffer, error_codes[i].desc);
        }
    }
#endif /* EFI_ERROR_MESSAGES */
    char16_t* p = buffer;
    for (uint8_t i = (sizeof(efi_status_t) - 1) * 2; i--;)
    if (status < 0x10 << i) (*p++) = u'0';
    return value_to_hex_string(p, status);
}

static inline
void ptr_flush(
    struct print_state* ps
) {
    *ps->pos = 0;
    ps->output(ps->context, ps->buffer);
    ps->pos = ps->buffer;
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
void ptr_putc (
    struct print_state* ps,
    char16_t c
) {
    /* if this is a newline, add a carraige return */
    if (c == '\n') {
        ptr_putc (ps, '\r');
    }

    *ps->pos = c;
    ps->pos += 1;
    ps->len += 1;

    /* if at the end of the buffer, flush it */
    if (ps->pos >= ps->end) {
        ptr_flush(ps);
    }
}

static inline
char16_t ptr_getc(
    struct ptr* p
) {
    char16_t c = 0;
    c = p->is_ascii ? p->pb[p->index] : p->pu[p->index];
    p->index++;

    return  c;
}

static inline
void ptr_item (
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

/*
    %w.lF   -   w = width
                l = field width
                F = format of arg
  Args F:
    0       -   pad with zeros
    -       -   justify on left (default is on right)
    *       -   width provided on stack
    n       -   Set output attribute to normal (for this field only)
    h       -   Set output attribute to highlight (for this field only)
    e       -   Set output attribute to error (for this field only)
    l       -   Value is 64 bits
    a       -   ascii string
    s       -   unicode string
    X       -   fixed 8 byte value in hex
    x       -   hex value
    d       -   value as signed decimal
    u       -   value as unsigned decimal
    f       -   value as floating point
    c       -   Unicode char
    t       -   EFI time structure
    g       -   Pointer to GUID
    r       -   EFI status code (result code)
    D       -   pointer to Device Path with normal ending.
    N       -   Set output attribute to normal
    H       -   Set output attribute to highlight
    E       -   Set output attribute to error
    %       -   Print a %
 */
efi_size_t _print(
    struct print_state* ps
) {
    char16_t c;
    struct _pitem item;
    char16_t buffer[PRINT_STRING_LEN];

    ps->len = 0;
    ps->buffer = buffer;
    ps->pos = buffer;
    ps->end = buffer + PRINT_STRING_LEN - 1;
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

                case 'a':
                    item.fmt.pb = va_arg(ps->args, char8_t*);
                    item.fmt.is_ascii = true;
                    if (!item.fmt.pb) {
                        item.fmt.pb = (char8_t*) "(null)";
                    }
                    break;

                case 's':
                    item.fmt.pu = va_arg(ps->args, char16_t*);
                    if (!item.fmt.pu) {
                        item.fmt.pu = u"(null)";
                    }
                    break;

                case 'c':
                    item.scratch[0] = (char16_t) va_arg(ps->args, efi_size_t);
                    item.scratch[1] = 0;
                    item.fmt.pu = item.scratch;
                    break;
                
                case 'l':
                    item.is_long = true;
                    break;

                case 'X':
                    item.width = item.is_long ? 16 : 8;
                    item.pad_char = u'0';
                    break;

                [[ fallthrough ]];
                case 'x':
                    value_to_hex_string (
                        item.scratch,
                        item.is_long ? va_arg(ps->args, uint64_t) : va_arg(ps->args, uint32_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'g':
                    guid_to_string (item.scratch, va_arg(ps->args, efi_guid_t*));
                    item.fmt.pu = item.scratch;
                    break;

                case 'u':
                    value_to_string (
                        item.scratch,
                        item.is_long ? va_arg(ps->args, uint64_t) : va_arg(ps->args, uint32_t));
                    item.fmt.pu = item.scratch;
                    break;

                case 'i':
                case 'd':
                    value_to_string (
                        item.scratch,
                        item.is_long ? va_arg(ps->args, uint64_t) : va_arg(ps->args, int32_t));
                    item.fmt.pu = item.scratch;
                    break;

#if EFI_FLOATING_POINT
                case 'f':
                    float_to_string (
                        item.scratch,
                        va_arg(ps->args, double)
                        );
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

                case 'h':
                    ptr_setattr(ps, ps->attr_highlight);
                    break;

                case 'e':
                    ptr_setattr(ps, ps->attr_error);
                    break;

                case 'N':
                    attr = ps->attr_norm;
                    break;

                case 'H':
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
    struct print_state ps = { 0 };
    efi_size_t ret;

    ps.context = out;
    ps.output = (output_string) out->output_string;
    ps.set_attr = (output_setattr) out->set_attribute;
    ps.attr = out->mode->attribute;

    ret = (ps.attr >> 4) & 0xf;
    ps.attr_norm = EFI_TEXT_ATTR(EFI_LIGHTGRAY, ret);
    ps.attr_highlight = EFI_TEXT_ATTR(EFI_WHITE, ret);
    ps.attr_error = EFI_TEXT_ATTR(EFI_YELLOW, ret);
    
    if (fmt) {
        ps.fmt.pu = fmt;
    } else {
        ps.fmt.is_ascii =true;
        ps.fmt.pb = fmta;
    }

    if (column != (efi_size_t) -1)
        out->set_cursor_position(out, column, row);

    va_copy(ps.args, args);
    ret = _print(&ps);
    va_end(ps.args);
    return ret;
}

#endif /* EFI_PRINTF */ 
