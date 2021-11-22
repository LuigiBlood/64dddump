
#include <ultra64.h>

#include "textlib.h"

#ifdef __MWERKS__
#pragma align(8)
#endif

u16 bitmap_buf1[SCREEN_SIZE_VY*SCREEN_SIZE_X] __attribute__ ((aligned (8)));
u16 bitmap_buf2[SCREEN_SIZE_VY*SCREEN_SIZE_X] __attribute__ ((aligned (8)));
u16 *bitmap_buf;
u8 backup_buffer[256*0x100] __attribute__ ((aligned (8)));





