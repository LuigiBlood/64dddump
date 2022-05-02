#ifndef __CONTROL_H__
#define __CONTROL_H__
#include	<ultra64.h>

#define	NUM_MESSAGE 	1
#define	CONT_VALID	1
#define	CONT_INVALID	0
#define WAIT_CNT	5

extern void initController();
extern void updateController();
extern u16 readControllerPressed();
extern u16 readControllerHold();

#endif /* __CONTROL_H__ */
