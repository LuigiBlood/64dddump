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
	s32 i, j;
	char console_text[256];
	
	osWritebackDCacheAll();

	initCartPi();
	
	detectCart();
	unlockCartWrite();

	initDisk();
	initController();

	process_init(0);

	while (1)
	{
		updateController();
		process_update();
		process_render();
	}

	for(;;);
}
