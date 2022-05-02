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

void mainproc(void *arg)
{
	int i, j;
	int error = 0;
	char console_text[50];
	
	osWritebackDCacheAll();

	//Init PI Manager
	initCartPi();

	dd_initText(4);

	
	//Initialize Leo Manager (64DD)
	//haxAll();
	//error = LeoCJCreateLeoManager((OSPri)OS_PRIORITY_LEOMGR-1, (OSPri)OS_PRIORITY_LEOMGR, LeoMessages, NUM_LEO_MESGS);
	//LeoResetClear();
	
	//Render Text
	dd_setScreenColor(0,0,50);
	dd_clearScreen();

	dd_setTextColor(255,255,255);
	dd_setTextPosition(20, 16);
	dd_printText("64DD Font Test\nI don't like Nintendo 64 development.\n\n");

	//Color Test
	dd_setTextColor(0,255,0);
	dd_printText("This ");
	dd_setTextColor(0,0,255);
	dd_printText("is ");
	dd_setTextColor(255,0,0);
	dd_printText("a ");
	dd_setTextColor(128,128,128);
	dd_printText("test.");
	dd_setTextColor(0,0,0);
	dd_printText(" TEST!");

	//Shadow Effect
	dd_setTextPosition(22, 18);
	dd_setTextColor(0,0,0);
	dd_printText("\n\n\n\nThis is only, a test.");

	dd_setTextPosition(20, 16);
	dd_setTextColor(16,128,255);
	dd_printText("\n\n\n\nThis is only, a test.");

	//Controller Test
	dd_loadTextFont(2);

	initController();

	while (1)
	{
		updateController();
		if (readControllerHold() != 0)
		{
			dd_setTextColor(0,255,0);
			if (i == 1)
			{
				dd_clearRect(0, 100, SCREEN_SIZE_X, 117);
				i = 0;
				dd_setTextPosition(80, 100);
				dd_printText("Controller Test");
			}
		}
		else
		{
			dd_setTextColor(255,0,0);
			if (i == 0)
			{
				dd_clearRect(0, 100, SCREEN_SIZE_X, 117);
				i = 1;
				dd_setTextPosition(80, 100);
				dd_printText("Controller Test");
			}
		}
	}

	for(;;);
}
