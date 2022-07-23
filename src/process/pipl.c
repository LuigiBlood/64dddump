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

#define PIPL_MODE_INIT		0
#define PIPL_MODE_CONFIRM	1
#define PIPL_MODE_DUMP		2
#define PIPL_MODE_FINISH	8

#define PIPL_SIZE		0x400000
#define PIPL_BLOCK_SIZE	0x4000
s32 pipl_dump_offset;
s32 pipl_dump_mode;

void pipl_f_progress(float percent)
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

void pipl_init()
{
	pipl_dump_offset = 0;
	pipl_dump_mode = PIPL_MODE_INIT;
}

void pipl_update()
{
	if (pipl_dump_mode == PIPL_MODE_INIT)
	{
		pipl_dump_mode = PIPL_MODE_CONFIRM;
	}
	else if (pipl_dump_mode == PIPL_MODE_CONFIRM)
	{
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
		if (readControllerPressed() & A_BUTTON)
		{
			pipl_dump_mode = PIPL_MODE_DUMP;
		}
	}
	else if (pipl_dump_mode == PIPL_MODE_DUMP)
	{
		iplBlockRead(pipl_dump_offset);
		osWritebackDCacheAll();
		copyToCartPi(blockData, (char*)pipl_dump_offset, PIPL_BLOCK_SIZE);
		pipl_dump_offset += PIPL_BLOCK_SIZE;
		if (pipl_dump_offset >= PIPL_SIZE)
			pipl_dump_mode = PIPL_MODE_FINISH;
	}
	else if (pipl_dump_mode == PIPL_MODE_FINISH)
	{
		//TODO: Write to SD Card

		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
}

void pipl_render(s32 fullrender)
{
	char console_text[256];

	if (fullrender)
	{
		dd_setScreenColor(10,10,0);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, SW_NAMESTRING);

		dd_setTextColor(255,255,0);
		dd_printText(FALSE, "Dump IPL ROM Mode\n");

		if (pipl_dump_mode == PIPL_MODE_CONFIRM)
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
		else if (pipl_dump_mode == PIPL_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", pipl_dump_offset, PIPL_SIZE);
			dd_printText(FALSE, console_text);

			pipl_f_progress((pipl_dump_offset / (float)PIPL_SIZE));
		}
		else if (pipl_dump_mode == PIPL_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", pipl_dump_offset, PIPL_SIZE);
			dd_printText(FALSE, console_text);
			dd_printText(FALSE, "IPL ROM Dumped.");

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");

			pipl_f_progress((pipl_dump_offset / (float)PIPL_SIZE));
		}
	}
}
