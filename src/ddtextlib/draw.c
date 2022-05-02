#include	<ultra64.h>
#include	<PR/leo.h>
#include	"ddtextlib.h"

u16 dd_color;
u16 dd_bg_color;
u16 dd_ascii_font;
u32 dd_ascii_font_diff;
u16 dd_screen_x;
u16 dd_screen_y;
u16 dd_screen_x_start;
u8 dd_afont_data[0x3000] __attribute__ ((aligned (8)));

//Set Text Color
void dd_setTextColor(u8 r, u8 g, u8 b)
{
	dd_color = GPACK_RGBA5551(r,g,b,1);
}

//Set Text Screen Position
void dd_setTextPosition(u16 x, u16 y)
{
	dd_screen_x = x;
	dd_screen_y = y;
}

//Get Current Text Position X
u16 dd_getTextPositionX()
{
	return dd_screen_x;
}

//Get Current Text Position Y
u16 dd_getTextPositionY()
{
	return dd_screen_y;
}

//Set Background Screen Color
void dd_setScreenColor(u8 r, u8 g, u8 b)
{
	dd_bg_color = GPACK_RGBA5551(r,g,b,1);
}

//Fill Screen with Background Color
void dd_clearScreen()
{
	u32 i;
	u16 *r;
	r = bitmap_buf;
	for (i = 0; i < SCREEN_SIZE_X * SCREEN_SIZE_Y; i++)
		*r++ = dd_bg_color;
}

//u8 dd_colorConv[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
u16 dd_colorConv[] = {
	GPACK_RGBA5551(0x00,0x00,0x00,1),
	GPACK_RGBA5551(0x11,0x11,0x11,1),
	GPACK_RGBA5551(0x22,0x22,0x22,1),
	GPACK_RGBA5551(0x33,0x33,0x33,1),
	GPACK_RGBA5551(0x44,0x44,0x44,1),
	GPACK_RGBA5551(0x55,0x55,0x55,1),
	GPACK_RGBA5551(0x66,0x66,0x66,1),
	GPACK_RGBA5551(0x77,0x77,0x77,1),
	GPACK_RGBA5551(0x88,0x88,0x88,1),
	GPACK_RGBA5551(0x99,0x99,0x99,1),
	GPACK_RGBA5551(0xAA,0xAA,0xAA,1),
	GPACK_RGBA5551(0xBB,0xBB,0xBB,1),
	GPACK_RGBA5551(0xCC,0xCC,0xCC,1),
	GPACK_RGBA5551(0xDD,0xDD,0xDD,1),
	GPACK_RGBA5551(0xEE,0xEE,0xEE,1),
	GPACK_RGBA5551(0xFF,0xFF,0xFF,1)
	};

int getRed(int x)
{
	return (x >> 11) & 0x1F;
}
int getGreen(int x)
{
	return (x >> 6) & 0x1F;
}
int getBlue(int x)
{
	return (x >> 1) & 0x1F;
}
u16 packColor(int r, int g, int b)
{
	return (r << 11 | g << 6 | b << 1 | 1);
}

void dd_printChar(char c)
{
	int i, j;
	int code, dx, dy, cy, offset, intensity, width, y_diff;
	double dr, dg, db, dc;

	//Special Characters
	if (c < 0x20)
	{
		if (c == '\n')
		{
			dd_screen_x = dd_screen_x_start;
			dd_screen_y += LINE_HEIGHT;
		}
		return;
	}

	//Regular Characters (4BPP) bitmap_buf
	code = (c - 0x20) + dd_ascii_font;
	offset = LeoGetAAdr(code, &dx, &dy, &cy) - dd_ascii_font_diff;
	width = ((dx+1)&~1);
	y_diff = LINE_HEIGHT - dy + (dy - cy);

	for (i = 0; i < dy; i++)
	{
		if ((i + dd_screen_y) >= SCREEN_SIZE_Y) break;

		for (j = 0; j < dx; j++)
		{
			if ((j + dd_screen_x) >= SCREEN_SIZE_X) break;

			intensity = dd_afont_data[offset + (((i * width) + j) / 2)];
			if ((j & 1) == 0) intensity >>= 4;
			intensity &= 0x0F;

			dc = bitmap_buf[((dd_screen_y + i + y_diff) * SCREEN_SIZE_X) + (dd_screen_x + j)];
			
			dr = getRed(dd_color) - getRed(dc);
			dg = getGreen(dd_color) - getGreen(dc);
			db = getBlue(dd_color) - getBlue(dc);
			
			dr /= 16.0;
			dg /= 16.0;
			db /= 16.0;

			dr = getRed(dc) + dr * intensity;
			dg = getGreen(dc) + dg * intensity;
			db = getBlue(dc) + db * intensity;

			bitmap_buf[((dd_screen_y + i + y_diff) * SCREEN_SIZE_X) + (dd_screen_x + j)] = packColor(dr, dg, db);
		}
	}
	osWritebackDCacheAll();

	dd_screen_x += dx + 1;
}

void dd_printText(char *s)
{
	dd_screen_x_start = dd_screen_x;
	while(*s) dd_printChar(*s++);
	osWritebackDCacheAll();
}
