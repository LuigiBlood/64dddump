#include <ultra64.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>
#include	"ddtextlib.h"

static const char *const str_cause[] =
{
    "Interrupt",
    "TLB modification",
    "TLB exception on load",
    "TLB exception on store",
    "Address error on load",
    "Address error on store",
    "Bus error on inst.",
    "Bus error on data",
    "System call exception",
    "Breakpoint exception",
    "Reserved instruction",
    "Coprocessor unusable",
    "Arithmetic overflow",
    "Trap exception",
    "Virtual coherency on inst.",
    "Floating point exception",
    "Watchpoint exception",
    "Virtual coherency on data",
};

static const char str_gpr[][2] =
{
    "AT", "V0", "V1",
    "A0", "A1", "A2", "A3",
    "T0", "T1", "T2", "T3",
    "T4", "T5", "T6", "T7",
    "S0", "S1", "S2", "S3",
    "S4", "S5", "S6", "S7",
    "T8", "T9", "GP", "SP",
    "S8", "RA",
};

extern OSThread *__osFaultedThread;
void faultproc(void *arg)
{
	OSMesgQueue mq;
    OSMesg msg[1];
    OSThread *t;
    u64 *p;
    int i;
    int cause;
	char console_text[256];

	//Wait for Fault
    osCreateMesgQueue(&mq, msg, 1);
    osSetEventMesg(OS_EVENT_FAULT, &mq, (OSMesg)0);
    osRecvMesg(&mq, NULL, OS_MESG_BLOCK);

	//Print Stuff
	t = __osFaultedThread;

	dd_setScreenColor(10, 0, 0);
	dd_setTextColor(255, 255, 255);
	dd_clearScreen();
	dd_setTextPosition(20, 16);

	cause = (t->context.cause & 0x7C) >> 2;
	if      (cause == 0x17) cause = 0x10;
    else if (cause == 0x1F) cause = 0x11;
	
	sprintf(console_text, " THREAD:%d  (%s)\n", t->id, str_cause[cause]);
	dd_printTextI(FALSE, 1, console_text);
	sprintf(
		console_text,
        " SR:%08X  PC:%08X  VA:%08X\n\n",
        t->context.sr, t->context.pc, t->context.badvaddr
    );
	dd_printTextI(FALSE, 1, console_text);

	for (i = 0, p = &t->context.at; i < 29; i++, p++)
    {
        dd_setTextPosition(20 + (80 * (i % 3)), dd_getTextPositionY());
        sprintf(console_text, " %.2s:%08X%c", str_gpr[i], (u32)*p, (i+1) % 3 ? ' ' : '\n');
		dd_printTextI(FALSE, 1, console_text);
    }

	dd_swapBuffer();
	//Loop
	for (;;);
}
