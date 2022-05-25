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

#define PIPL_MODE_INIT		0
#define PIPL_MODE_DUMP		1
#define PIPL_MODE_FINISH	2

#define PIPL_SIZE		0x400000
#define PIPL_BLOCK_SIZE	0x4000
s32 pipl_dump_offset;
s32 pipl_dump_mode;

void pipl_init()
{
	pipl_dump_offset = 0;
	pipl_dump_mode = PIPL_MODE_INIT;
}

void pipl_update()
{
	if (pipl_dump_mode == PIPL_MODE_INIT)
	{
		pipl_dump_mode = PIPL_MODE_DUMP;
	}
	else if (pipl_dump_mode == PIPL_MODE_DUMP)
	{
		iplBlockRead(pipl_dump_offset);
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
		dd_printText(FALSE, "64DD Dump Tool v0.1dev\n");

		dd_setTextColor(255,255,0);
		dd_printText(FALSE, "Dump IPL ROM Mode\n");

		if (pipl_dump_mode == PIPL_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", pipl_dump_offset, PIPL_SIZE);
			dd_printText(FALSE, console_text);
		}
		else if (pipl_dump_mode == PIPL_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", pipl_dump_offset, PIPL_SIZE);
			dd_printText(FALSE, console_text);
			dd_printText(FALSE, "IPL ROM Dumped\n\nPress the A Button\nto go back to the main menu.");
		}
	}
}
