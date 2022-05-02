//64DD Software Font Rendering

#ifndef _ddtextlib_h_
#define _ddtextlib_h_

#include	<ultra64.h>

extern u16      *bitmap_buf;
extern u16      bitmap_buf1[];
extern u16      bitmap_buf2[];

extern u8 _ddfontSegmentRomStart[];

extern u16 dd_color;
extern u16 dd_bg_color;
extern u16 dd_ascii_font;
extern u32 dd_ascii_font_diff;
extern u16 dd_screen_x;
extern u16 dd_screen_y;
extern u8 dd_afont_data[];

#define SCREEN_LOW  1
#define SCREEN_HIGH 2
#define RESOLUTION SCREEN_LOW

#define SCREEN_SIZE_X 320*RESOLUTION
#define SCREEN_SIZE_Y 240*RESOLUTION
#define SCREEN_SIZE_VY 240*2*RESOLUTION

#define LINE_HEIGHT 16

extern void dd_initText(u8 font);
extern void dd_clearScreen();

extern void dd_setTextColor(u8 r, u8 g, u8 b);
extern void dd_setTextPosition(u16 x, u16 y);
extern u16 dd_getTextPositionX();
extern u16 dd_getTextPositionY();
extern void dd_loadTextFont(u8 f);
extern void dd_setScreenColor(u8 r, u8 g, u8 b);

extern void dd_printText(char *s);

#endif
