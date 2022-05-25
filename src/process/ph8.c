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

#define PH8_MODE_INIT		0
#define PH8_MODE_CONFIRM	1
#define PH8_MODE_DUMP		2
#define PH8_MODE_FINISH		8

#define PH8_BASE		0x0000
#define PH8_SIZE		0x8000
#define PH8_BLOCK_SIZE	0x1000
s32 ph8_dump_offset;
s32 ph8_dump_mode;

void ph8_f_progress(float percent)
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

void ph8_init()
{
	ph8_dump_offset = 0;
	ph8_dump_mode = PH8_MODE_INIT;
}

void ph8_update()
{
	if (ph8_dump_mode == PH8_MODE_INIT)
	{
		ph8_dump_mode = PH8_MODE_CONFIRM;
	}
	else if (ph8_dump_mode == PH8_MODE_CONFIRM)
	{
		if (readControllerPressed() & B_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
		if (readControllerPressed() & A_BUTTON)
		{
			ph8_dump_mode = PH8_MODE_DUMP;
		}
	}
	else if (ph8_dump_mode == PH8_MODE_DUMP)
	{
		h8BlockRead(PH8_BASE + ph8_dump_offset);
		osWritebackDCacheAll();
		copyToCartPi(blockData, (char*)ph8_dump_offset, PH8_BLOCK_SIZE);
		ph8_dump_offset += PH8_BLOCK_SIZE;
		if (ph8_dump_offset >= PH8_SIZE)
			ph8_dump_mode = PH8_MODE_FINISH;
	}
	else if (ph8_dump_mode == PH8_MODE_FINISH)
	{
		//TODO: Write to SD Card

		//Interaction
		if (readControllerPressed() & A_BUTTON)
		{
			process_change(PROCMODE_PMAIN);
		}
	}
}

void ph8_render(s32 fullrender)
{
	char console_text[256];

	if (fullrender)
	{
		dd_setScreenColor(0,10,0);
		dd_clearScreen();

		dd_setTextColor(255,255,255);
		dd_setTextPosition(20, 16);
		dd_printText(FALSE, "64DD Dump Tool v0.1dev\n");

		dd_setTextColor(0,255,0);
		dd_printText(FALSE, "Dump H8 ROM Mode\n");

		if (ph8_dump_mode == PH8_MODE_CONFIRM)
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
		else if (ph8_dump_mode == PH8_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", ph8_dump_offset, PH8_SIZE);
			dd_printText(FALSE, console_text);

			ph8_f_progress((ph8_dump_offset / (float)PH8_SIZE));
		}
		else if (ph8_dump_mode == PH8_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", ph8_dump_offset, PH8_SIZE);
			dd_printText(FALSE, console_text);
			dd_printText(FALSE, "H8 ROM Dumped.");

			dd_setTextPosition(20, 16*8);
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, "Press ");
			dd_setTextColor(25,25,255);
			dd_printText(FALSE, "A Button");
			dd_setTextColor(255,255,255);
			dd_printText(FALSE, " to return to menu.");

			ph8_f_progress((ph8_dump_offset / (float)PH8_SIZE));
		}
	}
}
