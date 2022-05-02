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

//Print single character on screen
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
			//New Line
			dd_screen_x = dd_screen_x_start;
			dd_screen_y += LINE_HEIGHT;
		}
		return;
	}

	//Regular Characters (I4)
	//Proper Character Code for libleo
	code = (c - 0x20) + dd_ascii_font;
	//Get Offset and overall char info
	offset = LeoGetAAdr(code, &dx, &dy, &cy) - dd_ascii_font_diff;
	//Calculate proper image width
	width = ((dx+1)&~1);
	//Align chars on the same line
	y_diff = LINE_HEIGHT - dy + (dy - cy);

	for (i = 0; i < dy; i++)
	{
		//Avoid writing beyond the frame buffer
		if ((i + dd_screen_y + y_diff) >= SCREEN_SIZE_Y) break;

		for (j = 0; j < dx; j++)
		{
			//Avoid writing beyond the frame buffer
			if ((j + dd_screen_x) >= SCREEN_SIZE_X) break;

			//Get the Pixel Intensity Level
			intensity = dd_afont_data[offset + (((i * width) + j) / 2)];
			if ((j & 1) == 0) intensity >>= 4;
			intensity &= 0x0F;

			//Don't bother if there's no intensity (optimization)
			if (intensity == 0) continue;

			//Use color from frame buffer
			dc = bitmap_buf[((dd_screen_y + i + y_diff) * SCREEN_SIZE_X) + (dd_screen_x + j)];

			//Calculate the difference and gradually set intensity
			dr = getRed(dd_color) - getRed(dc);
			dg = getGreen(dd_color) - getGreen(dc);
			db = getBlue(dd_color) - getBlue(dc);
			
			dr /= 15.0;
			dg /= 15.0;
			db /= 15.0;

			dr = getRed(dc) + dr * intensity;
			dg = getGreen(dc) + dg * intensity;
			db = getBlue(dc) + db * intensity;

			//Write the new color to the frame buffer
			bitmap_buf[((dd_screen_y + i + y_diff) * SCREEN_SIZE_X) + (dd_screen_x + j)] = packColor(dr, dg, db);
		}
	}
	osWritebackDCacheAll();

	//Add char width + 1 to the current position
	dd_screen_x += dx + 1;
}

//Print string on screen
void dd_printText(char *s)
{
	dd_screen_x_start = dd_screen_x;
	while(*s) dd_printChar(*s++);
	osWritebackDCacheAll();
}
