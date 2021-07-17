#include "efilib/config.h"

#if EFI_FLOATING_POINT
/* marker to ignore float runtime */
int _fltused = 0;
#endif
