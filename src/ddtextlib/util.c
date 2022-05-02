#include	<ultra64.h>
#include	"ddtextlib.h"

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
