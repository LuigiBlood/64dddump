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

void pipl_init()
{

}

void pipl_update()
{
	
}

void pipl_render(s32 fullrender)
{
	if (fullrender)
	{
		dd_setScreenColor(10,10,0);
		dd_clearScreen();
	}
}
