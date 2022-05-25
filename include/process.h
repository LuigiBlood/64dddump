#ifndef _process_h_
#define _process_h_

#include	<ultra64.h>
#include	<nustd/stdlib.h>

#define PROCMODE_PMAIN	0
#define PROCMODE_PDISK	1
#define PROCMODE_PIPL	2
#define PROCMODE_PH8	3
#define PROCMODE_PEEP	4
#define PROCMODE_PCONF	5

//Main Process
extern void process_first(s32 mode);
extern void process_change(s32 mode);
extern s32 process_check();

extern void process_init(s32 mode);
extern void process_update();
extern void process_render(s32 fullrender);

//pmain
extern void pmain_init();
extern void pmain_update();
extern void pmain_render(s32 fullrender);

//pipl
extern void pipl_init();
extern void pipl_update();
extern void pipl_render(s32 fullrender);

//ph8
extern void ph8_init();
extern void ph8_update();
extern void ph8_render(s32 fullrender);

//peep
extern void peep_init();
extern void peep_update();
extern void peep_render(s32 fullrender);

#endif
