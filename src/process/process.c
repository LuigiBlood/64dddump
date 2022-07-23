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

s32 proc_mode;
s32 proc_mode_next;

//Initialize Main Process
void process_first(s32 mode)
{
	dd_initText(4);

	process_change(mode);
	process_init(mode);
}

//Change Main Process Mode
void process_change(s32 mode)
{
	proc_mode_next = mode;
}

//Check if process mode is changing, and if so, initialize the new one
s32 process_check()
{
	if (proc_mode_next == proc_mode)
		return 0;
	
	process_init(proc_mode_next);
	return 1;
}

//Initialize Main Process
void process_init(s32 mode)
{
	proc_mode = mode;
	switch (proc_mode)
	{
		case PROCMODE_PMAIN:
			pmain_init();
			break;
		case PROCMODE_PDISK:
			pdisk_init();
			break;
		case PROCMODE_PIPL:
			pipl_init();
			break;
		case PROCMODE_PH8:
			ph8_init();
			break;
		case PROCMODE_PEEP:
			peep_init();
			break;
	}
}

//Main Update Process
void process_update()
{
	switch (proc_mode)
	{
		case PROCMODE_PMAIN:
			pmain_update();
			break;
		case PROCMODE_PDISK:
			pdisk_update();
			break;
		case PROCMODE_PIPL:
			pipl_update();
			break;
		case PROCMODE_PH8:
			ph8_update();
			break;
		case PROCMODE_PEEP:
			peep_update();
			break;
	}
}

//Main Render Process
void process_render(s32 fullrender)
{
	switch (proc_mode)
	{
		case PROCMODE_PMAIN:
			pmain_render(fullrender);
			break;
		case PROCMODE_PDISK:
			pdisk_render(fullrender);
			break;
		case PROCMODE_PIPL:
			pipl_render(fullrender);
			break;
		case PROCMODE_PH8:
			ph8_render(fullrender);
			break;
		case PROCMODE_PEEP:
			peep_render(fullrender);
			break;
	}
}
