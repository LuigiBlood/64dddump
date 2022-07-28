#ifndef _crc32_h_
#define _crc32_h_

extern u32 crc32calc;

extern u32 crc32(const u8 *input, s32 size);
extern u32 crc32calc_start();
extern u32 crc32calc_proc(u8 byte);
extern u32 crc32calc_procarray(const u8 *input, s32 size);
extern u32 crc32calc_end();

#endif
