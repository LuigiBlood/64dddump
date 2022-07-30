#include	<ultra64.h>
#include	<PR/leo.h>
#include	"ddtextlib.h"
#include	"cartaccess.h"

s32 cur_framebuffer;

//Initialize Screen
void dd_initText()
{
	u32 mode = RESOLUTION;
	u32 i;
	u16 *r1;
	u16 *r2;

	dd_setTextColor(255, 255, 255);
	dd_setScreenColor(0, 0, 0);
	dd_setTextPosition(0, 0);
	
	bitmap_buf = bitmap_buf1;
	cur_framebuffer = 0;
	if(mode == SCREEN_LOW)
	{
		osViSetMode(&osViModeTable[OS_VI_NTSC_LPF1]);
	}
	else
	{
		osViSetMode(&osViModeTable[OS_VI_NTSC_HPF1]);
	}

  	osViSwapBuffer(bitmap_buf);

	r1 = bitmap_buf1;
	r2 = bitmap_buf2;
	for (i = 0; i < SCREEN_SIZE_X * SCREEN_SIZE_Y; i++) 
	{
		*r1++ = dd_bg_color;
		*r2++ = dd_bg_color;
	}
}

void dd_swapBuffer()
{
	osViSwapBuffer(bitmap_buf);
	if (cur_framebuffer == 0)
	{
		bitmap_buf = bitmap_buf2;
		cur_framebuffer = 1;
	}
	else
	{
		bitmap_buf = bitmap_buf1;
		cur_framebuffer = 0;
	}
}

//Preload ASCII Text Font into buffer
void dd_loadTextFont(s32 id, u8 f)
{
	int dummy, i;
	int fnt_start, fnt_end, fnt_size;

	if (id >= FONT_CACHE_AMOUNT) return;

	dd_afont[id] = f * 0xC0;

	fnt_start = LeoGetAAdr(dd_afont[id], &dummy, &dummy, &dummy);
	fnt_end = LeoGetAAdr(dd_afont[id] + 0xC0, &dummy, &dummy, &dummy);
	fnt_size = fnt_end - fnt_start;

	dd_afont_diff[id] = fnt_start;

	osWritebackDCacheAll();
	copyFromCartPi(_ddfontSegmentRomStart + fnt_start, dd_afont_data + (0x3000 * id), 0x3000);
}
