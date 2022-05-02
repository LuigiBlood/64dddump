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
	
	//Initalize Console-type screen
	osWritebackDCacheAll();

	//Init PI Manager
	initCartPi();

	//init_draw();
	dd_initText(4);
	
	//Initial Text
	//setcolor(0,255,0);
	//draw_puts("\f\n    If you see this for a long period of time,\nsomething has messed up very badly.");

	
	//Initialize Leo Manager (64DD)
	//haxAll();
	//error = LeoCJCreateLeoManager((OSPri)OS_PRIORITY_LEOMGR-1, (OSPri)OS_PRIORITY_LEOMGR, LeoMessages, NUM_LEO_MESGS);
	//LeoResetClear();
	
	//setbgcolor(15,15,15);
	//clear_draw();
	
	//Render Text
	//setcolor(255,255,255);
	//draw_puts("\f\n    64DD Dump Tool 1.0\n    ------------------------------\n");
	dd_setScreenColor(0,0,50);
	dd_clearScreen();

	dd_setTextColor(255,255,255);
	dd_setTextPosition(20, 16);
	dd_printText("64DD Font Test\nI don't like Nintendo 64 development.\n\n");

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

	dd_setTextPosition(20, 16);
	dd_setTextColor(16,128,255);
	dd_printText("\n\n\n\nThis is only, a test.");

	/*initController();

	while (1)
	{
		updateController();
		if (readControllerHold() != 0)
		{
			setcolor(0,255,0);
		}
		else
		{
			setcolor(255,0,0);
		}
		draw_puts("\f\n\n\n    TEST");
	}*/

	for(;;);
}
