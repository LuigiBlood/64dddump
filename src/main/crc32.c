#include	<ultra64.h>
#include	"crc32.h"

u32 crc32calc;

u32 crc32_for_byte(u32 byte)
{
	const u32 polynomial = 0xEDB88320;
	u32 result = byte;
	s32 i = 0;

	for (; i < 8; i++)
	{
		result = (result >> 1) ^ (result & 1) * polynomial;
	}
	return result;
}

u32 crc32(const u8 *input, s32 size)
{
	u32 result = 0xFFFFFFFF;
	s32 i = 0;

	for (; i < size; i++) {
		result ^= input[i];
		result = crc32_for_byte(result);
	}

	return ~result;
}

u32 crc32calc_start()
{
	crc32calc = 0xFFFFFFFF;
}

u32 crc32calc_proc(u8 byte)
{
	crc32calc ^= byte;
	crc32calc = crc32_for_byte(crc32calc);
}

u32 crc32calc_procarray(const u8 *input, s32 size)
{
	s32 i = 0;
	for (; i < size; i++) {
		crc32calc ^= input[i];
		crc32calc = crc32_for_byte(crc32calc);
	}
}

u32 crc32calc_end()
{
	crc32calc = ~crc32calc;
}
