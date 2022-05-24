#ifndef __CART_H__
#define __CART_H__

#include	"64drive.h"

#define CART_TYPE_NONE		-1
#define CART_TYPE_64DRIVE	0
#define CART_TYPE_EVERDRIVE	1

extern OSMesgQueue dmaMessageQ;
extern OSPiHandle *pi_handle;
extern s32 cart_type;
#define cartGetType()	(cart_type)

extern void initCartPi();
extern void detectCart();
extern void unlockCartWrite();
extern void copyToCartPi(char *src, char *dest, const int len);
extern void copyFromCartPi(char *src, char *dest, const int len);
extern u32 cartRead(u32 addr);

#endif /* __CART_H__ */
