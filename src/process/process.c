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

s32 proc_mode;

//Initialize Main Process
void process_init(s32 mode)
{
	dd_initText(4);
	process_change(mode);
}

//Change Main Process Mode
void process_change(s32 mode)
{
	proc_mode = mode;
	switch (proc_mode)
	{
		case 0:
			pmain_init();
			break;
	}
}

//Main Update Process
void process_update()
{
	switch (proc_mode)
	{
		case 0:
			pmain_update();
			break;
	}
}

//Main Render Process
void process_render()
{
	switch (proc_mode)
	{
		case 0:
			pmain_render();
			break;
	}
}
