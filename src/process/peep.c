#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>

#include	"thread.h"
#include	"ddtextlib.h"

#include	"leohax.h"
#include	"cartaccess.h"
#include	"control.h"
#include	"leoctl.h"
#include	"64drive.h"
#include	"process.h"
#include	"version.h"
#include	"crc32.h"
#include	"global.h"

#include	"ff.h"

#define PEEP_MODE_INIT		0
#define PEEP_MODE_CONFIRM	1
#define PEEP_MODE_DUMP		2
#define PEEP_MODE_SAVE		3
#define PEEP_MODE_FINISH	8

#define PEEP_SIZE		0x80
s32 peep_dump_offset;
//s32 peep_dump_mode;
//s32 peep_dump_error;
//FRESULT peep_dump_error2;

void peep_f_progress(float percent)
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
	dd_printChar(FALSE, 0, '%');
}

void peep_init()
{
	peep_dump_offset = 0;
	proc_sub_dump_mode = PEEP_MODE_INIT;
	proc_sub_dump_error = 0;
	proc_sub_dump_error2 = FR_OK;
	bzero(blockData, PEEP_SIZE);
}

void peep_update()
{
	if (proc_sub_dump_mode == PEEP_MODE_INIT)
	{
		proc_sub_dump_mode = PEEP_MODE_CONFIRM;
	}
	else if (proc_sub_dump_mode == PEEP_MODE_CONFIRM)
	{
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
		if (readControllerPressed() & A_BUTTON)
		{
			crc32calc_start();
			proc_sub_dump_mode = PEEP_MODE_DUMP;
		}
	}
	else if (proc_sub_dump_mode == PEEP_MODE_DUMP)
	{
		eepromBlockRead();
		osWritebackDCacheAll();
		copyToCartPi(blockData, (char*)peep_dump_offset, PEEP_SIZE);
		crc32calc_procarray(blockData, PEEP_SIZE);
		peep_dump_offset += PEEP_SIZE;
		if (peep_dump_offset >= PEEP_SIZE)
		{
			crc32calc_end();
			if (conf_sdcardwrite == 1)
			{
				if (drivetype == LEO_DRIVE_TYPE_RETAIL)
					makeUniqueFilename("/dump/DDEEP_R", "rom");
				else if (drivetype == LEO_DRIVE_TYPE_DEV)
					makeUniqueFilename("/dump/DDEEP_D", "rom");
				else if (drivetype == LEO_DRIVE_TYPE_WRITER)
					makeUniqueFilename("/dump/DDEEP_W", "rom");
				else
					makeUniqueFilename("/dump/DDEEP_U", "rom");
			}
			proc_sub_dump_mode = PEEP_MODE_SAVE;
		}
	}
	else if (proc_sub_dump_mode == PEEP_MODE_SAVE)
	{
		FRESULT fr;
		int proc;

		if (conf_sdcardwrite == 1)
		{
			//Create dump folder if it doesn't exist
			fr = f_mkdir("dump");
			if (fr != FR_OK && fr != FR_EXIST)
			{
				proc_sub_dump_error = WRITE_ERROR_FMKDIR;
				proc_sub_dump_error2 = fr;
			}
			else
			{
				fr = writeFileROM(DumpPath, PEEP_SIZE, &proc);
				if (fr != FR_OK) proc_sub_dump_error = proc;
				proc_sub_dump_error2 = fr;
			}
		}
		proc_sub_dump_mode = PEEP_MODE_FINISH;
	}
	else if (proc_sub_dump_mode == PEEP_MODE_FINISH)
	{
		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
}

void peep_render(s32 fullrender)
{
	char console_text[256];

	if (fullrender)
	{
		dd_setScreenColor(0,0,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, SW_NAMESTRING);

		dd_setTextColor(30,30,255);
		dd_printText(FALSE, "Dump EEPROM Mode\n");

		if (proc_sub_dump_mode == PEEP_MODE_CONFIRM)
		{
			dd_setTextPosition(20, 16*7);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to confirm dump.");

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,255,25);
			dd_printText(FALSE, "B Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");
		}
		else if (proc_sub_dump_mode == PEEP_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", peep_dump_offset, PEEP_SIZE);
			dd_printText(FALSE, console_text);

			peep_f_progress((peep_dump_offset / (float)PEEP_SIZE));
		}
		else if (proc_sub_dump_mode == PEEP_MODE_SAVE)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", peep_dump_offset, PEEP_SIZE);
			dd_printText(FALSE, console_text);

			if (conf_sdcardwrite == 1)
			{
				dd_printText(FALSE, "Saving EEPROM file as\n");
				dd_printText(FALSE, DumpPath);
			}

			peep_f_progress((peep_dump_offset / (float)PEEP_SIZE));
		}
		else if (proc_sub_dump_mode == PEEP_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\nCRC32: %08X\n", peep_dump_offset, PEEP_SIZE, crc32calc);
			dd_printText(FALSE, console_text);
			
			if (conf_sdcardwrite == 1)
			{
				if (proc_sub_dump_error != WRITE_ERROR_OK)
				{
					if (proc_sub_dump_error == WRITE_ERROR_FOPEN)
						dd_printText(FALSE, "f_open() Error");
					else if (proc_sub_dump_error == WRITE_ERROR_FWRITE)
						dd_printText(FALSE, "f_write() Error");
					else if (proc_sub_dump_error == WRITE_ERROR_FCLOSE)
						dd_printText(FALSE, "f_close() Error");
					else if (proc_sub_dump_error == WRITE_ERROR_FMKDIR)
						dd_printText(FALSE, "f_mkdir() Error");

					sprintf(console_text, " %i", proc_sub_dump_error2);
					dd_printText(FALSE, console_text);
				}
				else
				{
					dd_printText(FALSE, "EEPROM Dumped as\n");
					dd_printText(FALSE, DumpPath);
				}
			}
			else
			{
				dd_printText(FALSE, "Dump 0x80 bytes from cart.\n");
			}

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");

			peep_f_progress((peep_dump_offset / (float)PEEP_SIZE));
		}
	}
}
