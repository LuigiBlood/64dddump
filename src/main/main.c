#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>
#include	<cart.h>

#include	"thread.h"
#include	"ddtextlib.h"

#include	"leohax.h"
#include	"cartaccess.h"
#include	"control.h"
#include	"leoctl.h"
#include	"64drive.h"
#include	"process.h"
#include	"usb.h"

extern OSMesgQueue	retraceMessageQ;
extern OSMesg		retraceMessageBuf;

void mainproc(void *arg)
{
	s32 frame = 0;
	osWritebackDCacheAll();

	initCartPi();
	
	cart_init();
	usb_initialize();
	initFatFs();

	initDisk();
	initController();

	process_first(0);

	while (1)
	{
		if (frame == 0)
		{
			updateController();

			process_check();
			process_update();
		}
		else
		{
			process_render(1);
			dd_swapBuffer();
		}

		osRecvMesg(&retraceMessageQ,NULL,OS_MESG_BLOCK);
		frame ^= 1;
	}

	for(;;);
}
