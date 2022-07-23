#ifndef __CARTACCESS_H__
#define __CARTACCESS_H__

#include	"64drive.h"

extern OSMesgQueue dmaMessageQ;
extern OSPiHandle *pi_handle;

extern void initCartPi();
extern void copyToCartPi(char *src, char *dest, const int len);
extern void copyFromCartPi(char *src, char *dest, const int len);
extern u32 cartRead(u32 addr);
extern void cartWrite(u32 addr, u32 data);

#define io_read(x)	cartRead(x)
#define io_write(x, y)	cartWrite(x, y)

#endif /* __CARTACCESS_H__ */
