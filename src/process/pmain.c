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

s32 firstrender;
s32 drivetype;
s32 test;

void pmain_init()
{
	firstrender = 0;
	test = 0;

	drivetype = diskGetDriveType();
}

void pmain_update()
{
	if (readControllerHold() != 0)
		test = 1;
	else
		test = 0;
}

void pmain_render()
{
	if (!firstrender)
	{
		dd_setScreenColor(0,10,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, "64DD Dump Tool v0.1\n");
		
		switch (drivetype)
		{
			case 0:
				dd_setTextColor(50,50,50);
				dd_printText(FALSE, "Retail Drive\n");
				break;
			case 1:
				dd_setTextColor(50,50,255);
				dd_printText(FALSE, "Development Drive\n");
				break;
			case 2:
				dd_setTextColor(110,110,110);
				dd_printText(FALSE, "Writer Drive\n");
				break;
		}

		firstrender = 1;
	}

	if (test != 0)
		dd_setTextColor(0,255,0);
	else
		dd_setTextColor(255,0,0);

	dd_setTextPosition(80, 100);
	dd_printText(TRUE, "Controller Test");
}
