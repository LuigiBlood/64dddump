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

#define PMAIN_SELECT_MIN	0
#define PMAIN_SELECT_MAX	3
s32 firstrender;
s32 drivetype;
s32 select;

void pmain_init()
{
	firstrender = 0;
	select = 0;

	drivetype = diskGetDriveType();
}

void pmain_update()
{
	if (readControllerPressed() & U_JPAD)
	{
		select--;
		if (select == 1 && drivetype > 0) select--;

		if (select < PMAIN_SELECT_MIN) select = PMAIN_SELECT_MAX;
	}
	if (readControllerPressed() & D_JPAD)
	{
		select++;
		if (select == 1) select++;

		if (select > PMAIN_SELECT_MAX) select = PMAIN_SELECT_MIN;
	}
}

void pmain_render()
{
	if (!firstrender)
	{
		dd_setScreenColor(0,10,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, "64DD Dump Tool v0.1dev\n");
		
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

		dd_setTextColor(255,255,255);
		dd_printText(FALSE, "\nSelect what to dump:");

		firstrender = 1;
	}

	dd_setTextPosition(80, 100);

	if (select == 0) dd_setTextColor(0,255,0);
	else dd_setTextColor(25,25,25);
	dd_printText(TRUE, "Dump Disk\n");

	if (select == 1) dd_setTextColor(0,255,0);
	else if (drivetype > 0) dd_setTextColor(128,25,25);
	else dd_setTextColor(25,25,25);
	dd_printText(TRUE, "Dump IPL ROM\n");

	if (select == 2) dd_setTextColor(0,255,0);
	else dd_setTextColor(25,25,25);
	dd_printText(TRUE, "Dump H8 ROM\n");

	if (select == 3) dd_setTextColor(0,255,0);
	else dd_setTextColor(25,25,25);
	dd_printText(TRUE, "Dump EEPROM\n");
}
