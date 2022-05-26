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

#define PDISK_MODE_INIT		0
#define PDISK_MODE_WAIT		1
#define PDISK_MODE_CHECK	2
#define PDISK_MODE_CONF1	3
#define PDISK_MODE_CONF2	4
#define PDISK_MODE_DUMP		7
#define PDISK_MODE_FINISH	8
s32 pdisk_dump_mode;
s32 pdisk_cur_lba;
s32 pdisk_drivetype;

#define PDISK_SELECT2_MIN	0
#define PDISK_SELECT2_MAX	2
s32 pdisk_select;

void pdisk_f_progress(float percent)
{
	char console_text[256];
	s32 temp = 220*percent;

	dd_setScreenColor(255,255,255);
	dd_clearRect(50-1, (16*12)-1, 50+220+1, (16*13)+1);
	dd_setScreenColor(10,10,0);
	dd_clearRect(50, (16*12), 50+220, (16*13));
	dd_setScreenColor(0,255,255);
	dd_clearRect(50, (16*12), 50 + temp, (16*13));

	sprintf(console_text, "%i", (s32)ceilf(percent*100));
	dd_setTextPosition(150, (16*12)-3);
	dd_setTextColor(255,80,0);
	dd_printText(FALSE, console_text);
	dd_printChar(FALSE, '%');
}

void pdisk_init()
{
	pdisk_cur_lba = 0;
	pdisk_drivetype = diskGetDriveType();
	pdisk_dump_mode = PDISK_MODE_INIT;
	pdisk_select = 0;
}

void pdisk_update()
{
	if (pdisk_dump_mode == PDISK_MODE_INIT)
	{
		pdisk_dump_mode = PDISK_MODE_WAIT;
	}
	else if (pdisk_dump_mode == PDISK_MODE_WAIT)
	{
		//Check for Disk presence
		if (isDiskPresent()) pdisk_dump_mode = PDISK_MODE_CHECK;

		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CHECK)
	{
		//Check for Disk presence
		if (!isDiskPresent()) pdisk_dump_mode = PDISK_MODE_WAIT;

		if (diskReadID() == LEO_STATUS_GOOD)
		{
			pdisk_dump_mode = PDISK_MODE_CONF1;
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CONF1)
	{
		//Check for Disk presence
		if (!isDiskPresent()) pdisk_dump_mode = PDISK_MODE_WAIT;

		if (readControllerPressed() & A_BUTTON)
		{
			pdisk_dump_mode = PDISK_MODE_CONF2;
			pdisk_select = 0;
			if (pdisk_drivetype != LEO_DRIVE_TYPE_RETAIL) pdisk_select = 1;
		}
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CONF2)
	{
		//Check for Disk presence
		if (!isDiskPresent()) pdisk_dump_mode = PDISK_MODE_WAIT;

		//Selection
		if (readControllerPressed() & U_JPAD)
		{
			pdisk_select--;
			if (pdisk_select == 0 && pdisk_drivetype != LEO_DRIVE_TYPE_RETAIL) pdisk_select--;
		}
		if (readControllerPressed() & D_JPAD)
		{
			pdisk_select++;
			if (pdisk_select > PDISK_SELECT2_MAX) pdisk_select = PDISK_SELECT2_MIN;
			if (pdisk_select == 0 && pdisk_drivetype != LEO_DRIVE_TYPE_RETAIL) pdisk_select++;
		}

		if (pdisk_select < PDISK_SELECT2_MIN) pdisk_select = PDISK_SELECT2_MAX;
		if (pdisk_select > PDISK_SELECT2_MAX) pdisk_select = PDISK_SELECT2_MIN;

		if (readControllerPressed() & A_BUTTON)
		{
			pdisk_dump_mode = PDISK_MODE_CONF1;
		}
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_DUMP)
	{
		s32 offset, error, size;

		offset = diskGetLBAOffset(pdisk_cur_lba);
		size = diskGetLBABlockSize(pdisk_cur_lba);
		error = diskReadLBA(pdisk_cur_lba);
		copyToCartPi(blockData, (char*)offset, size);
	}
	else if (pdisk_dump_mode == PDISK_MODE_FINISH)
	{
		//TODO: Write to SD Card

		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
}

void pdisk_render(s32 fullrender)
{
	char console_text[256];

	if (fullrender)
	{
		dd_setScreenColor(10,0,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, "64DD Dump Tool v0.1dev\n");

		dd_setTextColor(255,0,255);
		dd_printText(FALSE, "Dump Disk Mode\n");

		if (pdisk_dump_mode == PDISK_MODE_WAIT)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			dd_printText(FALSE, "Please insert a disk.\n");

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,255,25);
			dd_printText(FALSE, "B Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");
		}
		else if (pdisk_dump_mode == PDISK_MODE_CHECK)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			dd_printText(FALSE, "Checking the disk...\n");
		}
		else if (pdisk_dump_mode == PDISK_MODE_CONF1)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "Game Code: %c%c%c%c -- Version: %i\n", _diskId.gameName[0], _diskId.gameName[1], _diskId.gameName[2], _diskId.gameName[3], _diskId.gameVersion);
			dd_printText(FALSE, console_text);
			sprintf(console_text, "Write Date: %02x%02x/%02x/%02x %02x:%02x:%02x\n", _diskId.serialNumber.time.yearhi, _diskId.serialNumber.time.yearlo,
				_diskId.serialNumber.time.month, _diskId.serialNumber.time.day,
				_diskId.serialNumber.time.hour, _diskId.serialNumber.time.minute, _diskId.serialNumber.time.second);
			dd_printText(FALSE, console_text);
			if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_JPN)
				dd_printText(FALSE, "Disk Region: Japan\n");
			else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_USA)
				dd_printText(FALSE, "Disk Region: USA\n");
			else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_NONE)
				dd_printText(FALSE, "Disk Region: None\n");
			else
				dd_printText(FALSE, "Disk Region: Unknown\n");

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to continue...");

			dd_setTextPosition(20, 16*9);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,255,25);
			dd_printText(FALSE, "B Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");
		}
		else if (pdisk_dump_mode == PDISK_MODE_CONF2)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			dd_printText(TRUE, "Select Dump Mode:");

			dd_setTextPosition(60, 16*5);

			if (pdisk_select == 0) dd_setTextColor(0,255,0);
			else if (pdisk_drivetype != LEO_DRIVE_TYPE_RETAIL) dd_setTextColor(128,25,25);
			else dd_setTextColor(25,25,25);
			dd_printText(TRUE, "Dump with Skips (Retail Only)\n");

			if (pdisk_select == 1) dd_setTextColor(0,255,0);
			else dd_setTextColor(25,25,25);
			dd_printText(TRUE, "Dump Entire Disk\n");

			if (pdisk_select == 2) dd_setTextColor(0,255,0);
			else dd_setTextColor(25,25,25);
			dd_printText(TRUE, "Dump Entire Disk Faster\n");

			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*11);
			if (pdisk_select == 0) dd_printText(TRUE, "Dumps the retail disk, then\ntries to dump unformatted segments,\nif it fails, skips the segment.");
			else if (pdisk_select == 1) dd_printText(TRUE, "\nDumps the entire disk data and\nbruteforces through bad blocks.");
			else if (pdisk_select == 2) dd_printText(TRUE, "Dumps the entire disk data and\nbruteforces through bad blocks,\nallows to lower redundant error reads.");
		}
	}
}
