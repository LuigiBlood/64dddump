#ifndef _pdisk_extra_h_
#define _pdisk_extra_h_

extern s32 pdisk_lba_rom_end;
extern s32 pdisk_lba_ram_start;
extern s32 pdisk_lba_ram_end;

extern s32 pdisk_e_checkbounds();
extern s32 pdisk_e_skiplba(s32 lba_skip_max);
extern void pdisk_e_crc32();

#endif
