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
#include	"version.h"

#define PMAIN_SELECT_MIN	0
#define PMAIN_SELECT_MAX	3
s32 drivetype;
s32 select;
s32 iplrompresent;

void pmain_init()
{
	//select = 0;

	drivetype = diskGetDriveType();
	iplrompresent = isIPLROMPresent();
}

void pmain_update()
{
	//No interactions possible
	if (drivetype < LEO_DRIVE_TYPE_RETAIL && !iplrompresent)
		return;

	//Selection
	if (readControllerPressed() & U_JPAD)
	{
		select--;
		if (select == 1 && !iplrompresent) select--;
	}
	if (readControllerPressed() & D_JPAD)
	{
		select++;
		if (select == 1 && !iplrompresent) select++;
	}

	if (select < PMAIN_SELECT_MIN) select = PMAIN_SELECT_MAX;
	if (select > PMAIN_SELECT_MAX) select = PMAIN_SELECT_MIN;
	if (drivetype == LEO_DRIVE_TYPE_NONE && !iplrompresent) select = 1;

	//Interaction
	if (readControllerPressed() & A_BUTTON)
	{
		process_change(select + PROCMODE_PDISK);
	}
}

void pmain_render(s32 fullrender)
{
	if (fullrender)
	{
		dd_setScreenColor(0,10,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, SW_NAMESTRING);
		
		switch (drivetype)
		{
			case LEO_DRIVE_TYPE_RETAIL:
				dd_setTextColor(50,50,50);
				dd_printText(FALSE, "Retail Drive\n");
				break;
			case LEO_DRIVE_TYPE_DEV:
				dd_setTextColor(50,50,255);
				dd_printText(FALSE, "Development Drive\n");
				break;
			case LEO_DRIVE_TYPE_WRITER:
				dd_setTextColor(110,110,110);
				dd_printText(FALSE, "Writer Drive\n");
				break;
			default:
				dd_setTextColor(128,25,25);
				dd_printText(FALSE, "No Drive found\n");
		}

		if (drivetype >= LEO_DRIVE_TYPE_RETAIL || iplrompresent)
		{
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "\nSelect what to dump:");
		}

		if (drivetype < LEO_DRIVE_TYPE_RETAIL && !iplrompresent)
		{
			dd_setTextPosition(40, 100);

			dd_setTextColor(255,255,255);
			dd_printText(TRUE, "Please power off the Nintendo 64\nand insert a 64DD Disk Drive\nor/and an IPL ROM Cartridge.");
		}

		switch (cartGetType())
		{
			case CART_TYPE_64DRIVE:
				dd_setTextPosition(20, 210);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "64drive detected.");
				break;
			default:
				dd_setTextPosition(20, 210);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Unknown cartridge");
				break;
		}
	}

	if (drivetype >= LEO_DRIVE_TYPE_RETAIL || iplrompresent)
	{
		dd_setTextPosition(80, 100);

		if (select == 0) dd_setTextColor(0,255,0);
		else if (drivetype < LEO_DRIVE_TYPE_RETAIL) dd_setTextColor(128,25,25);
		else dd_setTextColor(25,25,25);
		dd_printText(TRUE, "Dump Disk\n");

		if (select == 1) dd_setTextColor(0,255,0);
		else if (!iplrompresent) dd_setTextColor(128,25,25);
		else dd_setTextColor(25,25,25);
		dd_printText(TRUE, "Dump IPL ROM\n");

		if (select == 2) dd_setTextColor(0,255,0);
		else if (drivetype < LEO_DRIVE_TYPE_RETAIL) dd_setTextColor(128,25,25);
		else dd_setTextColor(25,25,25);
		dd_printText(TRUE, "Dump H8 ROM\n");

		if (select == 3) dd_setTextColor(0,255,0);
		else if (drivetype < LEO_DRIVE_TYPE_RETAIL) dd_setTextColor(128,25,25);
		else dd_setTextColor(25,25,25);
		dd_printText(TRUE, "Dump EEPROM\n");
	}
}
