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

#define PEEP_MODE_INIT		0
#define PEEP_MODE_DUMP		1
#define PEEP_MODE_FINISH	2

#define PEEP_SIZE		0x80
s32 peep_dump_offset;
s32 peep_dump_mode;

void peep_init()
{
	peep_dump_offset = 0;
	peep_dump_mode = PEEP_MODE_INIT;
}

void peep_update()
{
	if (peep_dump_mode == PEEP_MODE_INIT)
	{
		peep_dump_mode = PEEP_MODE_DUMP;
	}
	else if (peep_dump_mode == PEEP_MODE_DUMP)
	{
		eepromBlockRead();
		copyToCartPi(blockData, (char*)peep_dump_offset, PEEP_SIZE);
		peep_dump_offset += PEEP_SIZE;
		if (peep_dump_offset >= PEEP_SIZE)
			peep_dump_mode = PEEP_MODE_FINISH;
	}
	else if (peep_dump_mode == PEEP_MODE_FINISH)
	{
		//TODO: Write to SD Card

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
		dd_printText(FALSE, "64DD Dump Tool v0.1dev\n");

		dd_setTextColor(30,30,255);
		dd_printText(FALSE, "Dump EEPROM Mode\n");

		if (peep_dump_mode == PEEP_MODE_DUMP)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", peep_dump_offset, PEEP_SIZE);
			dd_printText(FALSE, console_text);
		}
		else if (peep_dump_mode == PEEP_MODE_FINISH)
		{
			dd_setTextColor(255,255,255);
			dd_setTextPosition(20, 16*4);
			sprintf(console_text, "%X/%X bytes\n", peep_dump_offset, PEEP_SIZE);
			dd_printText(FALSE, console_text);
			dd_printText(FALSE, "EEPROM Dumped\n\nPress the A Button\nto go back to the main menu.");
		}
	}
}
