#ifndef _process_h_
#define _process_h_

//Main Process
extern void process_init(s32 mode);
extern void process_change(s32 mode);
extern void process_update();
extern void process_render();

//pmain
extern void pmain_init();
extern void pmain_update();
extern void pmain_render();

#endif
