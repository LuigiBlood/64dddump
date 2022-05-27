#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>

#include	"thread.h"
#include	"ddtextlib.h"

#include	"asicdrive.h"
#include	"leohax.h"
#include	"cart.h"
#include	"control.h"
#include	"leoctl.h"
#include	"64drive.h"
#include	"process.h"
#include	"version.h"

#define PDISK_MODE_INIT		0
#define PDISK_MODE_WAIT		1
#define PDISK_MODE_CHECK	2
#define PDISK_MODE_CONF1	3
#define PDISK_MODE_CONF2	4
#define PDISK_MODE_DUMP		7
#define PDISK_MODE_FATAL	8
#define PDISK_MODE_FINISH	10
s32 pdisk_dump_mode;

#define PDISK_SELECT2_MIN	0
#define PDISK_SELECT2_MAX	2
s32 pdisk_select;
s32 pdisk_select_dump;

s32 pdisk_cur_lba;
s32 pdisk_drivetype;
s32 pdisk_dump_pause;
s32 pdisk_error_count;
s32 pdisk_error_fatal;

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
	pdisk_drivetype = diskGetDriveType();
	pdisk_dump_mode = PDISK_MODE_INIT;
	
	bzero(blockData, USR_SECS_PER_BLK*SEC_SIZE_ZONE0);
	bzero(errorData, MAX_P_LBA);
}

void pdisk_update()
{
	if (pdisk_dump_mode == PDISK_MODE_INIT)
	{
		pdisk_dump_mode = PDISK_MODE_WAIT;
	}
	else if (pdisk_dump_mode == PDISK_MODE_WAIT)
	{
		//Mode: Wait for Disk inserted in the drive.

		//Check for Disk presence, reset when disk is ejected
		if (isDiskPresent()) pdisk_dump_mode = PDISK_MODE_CHECK;

		//Press B Button to quit
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CHECK)
	{
		//Mode: Checking the Disk Info

		//Check for Disk presence, reset when disk is ejected
		if (!isDiskPresent()) pdisk_dump_mode = PDISK_MODE_WAIT;

		//Read Disk ID would do a bunch of checks on top
		//TODO: What to do when this fails
		if (diskReadID() == LEO_STATUS_GOOD)
		{
			pdisk_dump_mode = PDISK_MODE_CONF1;
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CONF1)
	{
		//Mode: Show Disk Info

		//Check for Disk presence, reset when disk is ejected
		if (!isDiskPresent()) pdisk_dump_mode = PDISK_MODE_WAIT;

		//Press A Button to continue
		if (readControllerPressed() & A_BUTTON)
		{
			pdisk_dump_mode = PDISK_MODE_CONF2;
			pdisk_select = 0;
			if (pdisk_drivetype != LEO_DRIVE_TYPE_RETAIL) pdisk_select = 1;
		}
		//Press B Button to quit
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_CONF2)
	{
		//Mode: Menu for Dump Type (Skip, Full, Full Fast)

		//Check for Disk presence, reset when disk is ejected
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

		//Press A Button to start dump
		if (readControllerPressed() & A_BUTTON)
		{
			pdisk_dump_mode = PDISK_MODE_DUMP;
			pdisk_select_dump = pdisk_select;
			pdisk_cur_lba = 0;
			pdisk_dump_pause = 0;
			pdisk_error_count = 0;
		}
		//Press B Button to quit
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_DUMP)
	{
		//Mode: Dump Mode Process
		s32 offset, error, size;

		if (readControllerPressed() & START_BUTTON)
		{
			pdisk_dump_pause ^= 1;
		}

		if (!pdisk_dump_pause)
		{
			offset = diskGetLBAOffset(pdisk_cur_lba);
			size = diskGetLBABlockSize(pdisk_cur_lba);
			error = diskReadLBA(pdisk_cur_lba);
			copyToCartPi(blockData, (char*)offset, size);

			switch (error)
			{
				case LEO_SENSE_DRIVE_NOT_READY:
				case LEO_SENSE_DIAGNOSTIC_FAILURE:
				case LEO_SENSE_COMMAND_PHASE_ERROR:
				case LEO_SENSE_DATA_PHASE_ERROR:
				case LEO_SENSE_UNKNOWN_FORMAT:
				case LEO_SENSE_DEVICE_COMMUNICATION_FAILURE:
				case LEO_SENSE_MEDIUM_NOT_PRESENT:
				case LEO_SENSE_POWERONRESET_DEVICERESET_OCCURED:
				case LEO_SENSE_MEDIUM_MAY_HAVE_CHANGED:
				case LEO_SENSE_EJECTED_ILLEGALLY_RESUME:
					pdisk_error_fatal = error;
					pdisk_dump_mode = PDISK_MODE_FATAL;
				case LEO_SENSE_NO_ADDITIONAL_SENSE_INFOMATION:
					break;
				case LEO_SENSE_NO_SEEK_COMPLETE:
				case LEO_SENSE_WRITE_FAULT:
				case LEO_SENSE_UNRECOVERED_READ_ERROR:
				case LEO_SENSE_NO_REFERENCE_POSITION_FOUND:
				case LEO_SENSE_TRACK_FOLLOWING_ERROR:
				default:
					pdisk_error_count++;
			}

			pdisk_cur_lba++;
		}
		else
		{
			if (readControllerPressed() & U_CBUTTONS)
			{
				//Skip to RAM or end
			}
			if (readControllerPressed() & B_BUTTON)
			{
				process_change(PROCMODE_PMAIN);
			}
		}

		if (pdisk_cur_lba > MAX_P_LBA)
		{
			pdisk_dump_mode = PDISK_MODE_FINISH;
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_FATAL)
	{
		//Mode: Fatal Error
		
		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
	else if (pdisk_dump_mode == PDISK_MODE_FINISH)
	{
		//Mode: Dump is done

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
		dd_printText(FALSE, SW_NAMESTRING);

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
			else if (pdisk_select == 2) dd_printText(TRUE, "Dumps the entire disk data and\nbruteforces through bad blocks,\nallows to lower repeated error reads.");
		}
		else if (pdisk_dump_mode == PDISK_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%i", pdisk_cur_lba);
			dd_printText(FALSE, console_text);

			dd_setTextPosition(58, 16*4);
			sprintf(console_text, "/%i blocks\n", MAX_P_LBA);
			dd_printText(FALSE, console_text);

			if (pdisk_error_count)
			{
				dd_setTextColor(255,80,25);
				dd_setTextPosition(20, 16*5);
				sprintf(console_text, "%i errors found", pdisk_error_count);
				dd_printText(FALSE, console_text);
			}

			if (!pdisk_dump_pause)
			{
				dd_setTextPosition(20, 16*8);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Press ");
				dd_setTextColor(50,50,50);
				dd_printText(FALSE, "START Button");
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, " to pause the dump.");
			}
			else
			{
				dd_setTextPosition(20, 16*6);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Paused");

				dd_setTextPosition(20, 16*8);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Press ");
				dd_setTextColor(50,50,50);
				dd_printText(FALSE, "START Button");
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, " to resume the dump.");

				dd_setTextPosition(20, 16*9);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Press ");
				dd_setTextColor(255,255,25);
				dd_printText(FALSE, "C Button Up");
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, " to skip to RAM Area.");

				dd_setTextPosition(20, 16*10);
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, "Press ");
				dd_setTextColor(25,255,25);
				dd_printText(FALSE, "B Button");
				dd_setTextColor(255,255,255);
				dd_printText(FALSE, " to cancel the dump.");
			}

			pdisk_f_progress((pdisk_cur_lba / (float)MAX_P_LBA));
		}
		else if (pdisk_dump_mode == PDISK_MODE_FATAL)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%i", pdisk_cur_lba);
			dd_printText(FALSE, console_text);

			dd_setTextPosition(58, 16*4);
			sprintf(console_text, "/%i blocks\n", MAX_P_LBA);
			dd_printText(FALSE, console_text);

			if (pdisk_error_count)
			{
				dd_setTextColor(255,80,25);
				dd_setTextPosition(20, 16*5);
				sprintf(console_text, "%i errors found", pdisk_error_count);
				dd_printText(FALSE, console_text);
			}

			dd_setTextColor(255,80,25);
			dd_setTextPosition(20, 16*7);
			dd_printText(FALSE, "FATAL ERROR:\n");
			sprintf(console_text, "%i / %s", pdisk_error_fatal, diskErrorString(pdisk_error_fatal));
			dd_printText(FALSE, console_text);

			dd_setTextPosition(20, 16*10);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to go back to menu.");

			pdisk_f_progress((pdisk_cur_lba / (float)MAX_P_LBA));
		}
		else if (pdisk_dump_mode == PDISK_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%i", MAX_P_LBA);
			dd_printText(FALSE, console_text);

			dd_setTextPosition(58, 16*4);
			sprintf(console_text, "/%i blocks\n", MAX_P_LBA);
			dd_printText(FALSE, console_text);

			dd_setTextPosition(20, 16*10);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to go back to menu.");

			pdisk_f_progress(1.0f);
		}
	}
}
