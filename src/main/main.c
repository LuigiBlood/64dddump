//64DD Dump 1.0
//Based on pfs SDK demo

#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>

#include	"thread.h"
#include	"ddtextlib.h"

#include	"leohax.h"
#include	"cart.h"
#include	"control.h"
#include	"leoctl.h"
#include	"64drive.h"
#include	"process.h"

void mainproc(void *arg)
{
	s32 frame = 0;
	osWritebackDCacheAll();

	initCartPi();
	
	detectCart();
	unlockCartWrite();

	initDisk();
	initController();

	process_first(0);

	while (1)
	{
		updateController();

		if (process_check()) frame = -1;
		process_update();
		process_render(frame < 2);
		dd_swapBuffer();

		if (frame < 2) frame++;
	}

	for(;;);
}
