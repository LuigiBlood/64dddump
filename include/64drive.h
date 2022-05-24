#ifndef _64drive_h_
#define _64drive_h_

#include <ultra64.h>
#include "cart.h"

#define D64_CIBASE_ADDRESS	0xB8000000
#define D64_REGISTER_MAGIC	0x000002EC
#define D64_MAGIC			0x55444556

extern void ciEnableRomWrites();

#endif
