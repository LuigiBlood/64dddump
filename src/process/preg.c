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

#define PREG_MODE_INIT		0
#define PREG_MODE_CONFIRM	1
#define PREG_MODE_DUMP		2
#define PREG_MODE_SAVE		3
#define PREG_MODE_FINISH	8

#define PREG_BASE		0x0000
#define PREG_SIZE		0x1000
s32 preg_dump_offset;
//s32 preg_dump_mode;
//s32 preg_dump_error;
//FRESULT preg_dump_error2;

void preg_f_progress(float percent)
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

void preg_init()
{
	preg_dump_offset = 0;
	proc_sub_dump_mode = PREG_MODE_INIT;
	proc_sub_dump_error = 0;
	proc_sub_dump_error2 = FR_OK;
	crc32calc_start();
}

void preg_update()
{
	if (proc_sub_dump_mode == PREG_MODE_INIT)
	{
		proc_sub_dump_mode = PREG_MODE_CONFIRM;
	}
	else if (proc_sub_dump_mode == PREG_MODE_CONFIRM)
	{
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
		if (readControllerPressed() & A_BUTTON)
		{
			proc_sub_dump_mode = PREG_MODE_DUMP;
		}
	}
	else if (proc_sub_dump_mode == PREG_MODE_DUMP)
	{
		registerBlockDump();
		osWritebackDCacheAll();
		copyToCartPi(blockData, (char*)preg_dump_offset, PREG_SIZE);
		crc32calc_procarray(blockData, PREG_SIZE);
		preg_dump_offset += PREG_SIZE;
		if (preg_dump_offset >= PREG_SIZE)
		{
			crc32calc_end();
			if (conf_sdcardwrite == 1)
			{
				if (drivetype == LEO_DRIVE_TYPE_RETAIL)
					makeUniqueFilename("/dump/DDReg_R", "rom");
				else if (drivetype == LEO_DRIVE_TYPE_DEV)
					makeUniqueFilename("/dump/DDReg_D", "rom");
				else if (drivetype == LEO_DRIVE_TYPE_WRITER)
					makeUniqueFilename("/dump/DDReg_W", "rom");
				else
					makeUniqueFilename("/dump/DDReg_U", "rom");
			}
			proc_sub_dump_mode = PREG_MODE_SAVE;
		}
	}
	else if (proc_sub_dump_mode == PREG_MODE_SAVE)
	{
		FRESULT fr;
		int proc;

		if (conf_sdcardwrite == 1)
		{
			fr = writeFileROM(DumpPath, PREG_SIZE, &proc);
			if (fr != FR_OK) proc_sub_dump_error = proc;
			proc_sub_dump_error2 = fr;
		}
		proc_sub_dump_mode = PREG_MODE_FINISH;
	}
	else if (proc_sub_dump_mode == PREG_MODE_FINISH)
	{
		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
}

void preg_render(s32 fullrender)
{
	char console_text[256];

	if (fullrender)
	{
		dd_setScreenColor(0,10,10);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, SW_NAMESTRING);

		dd_setTextColor(0,255,255);
		dd_printText(FALSE, "Dump Register Mode\n");

		if (proc_sub_dump_mode == PREG_MODE_CONFIRM)
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
		else if (proc_sub_dump_mode == PREG_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", preg_dump_offset, PREG_SIZE);
			dd_printText(FALSE, console_text);

			preg_f_progress((preg_dump_offset / (float)PREG_SIZE));
		}
		else if (proc_sub_dump_mode == PREG_MODE_SAVE)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", preg_dump_offset, PREG_SIZE);
			dd_printText(FALSE, console_text);
			
			if (conf_sdcardwrite == 1)
			{
				dd_printText(FALSE, "Saving Register dump as\n");
				dd_printText(FALSE, DumpPath);
			}

			preg_f_progress((preg_dump_offset / (float)PREG_SIZE));
		}
		else if (proc_sub_dump_mode == PREG_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\nCRC32: %08X\n", preg_dump_offset, PREG_SIZE, crc32calc);
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

					sprintf(console_text, " %i", proc_sub_dump_error2);
					dd_printText(FALSE, console_text);
				}
				else
				{
					dd_printText(FALSE, "Registers Dumped as\n");
					dd_printText(FALSE, DumpPath);
				}
			}
			else
			{
				dd_printText(FALSE, "Dump 0x1000 bytes from cart.\n");
			}

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");

			preg_f_progress((preg_dump_offset / (float)PREG_SIZE));
		}
	}
}
